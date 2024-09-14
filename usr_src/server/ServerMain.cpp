#include "ServerMain.hpp"
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <functional>
#include <fstream>


ServerMain::ServerMain() {
    Init();
}

ServerMain::~ServerMain() {
    StopServer();
    if (ServerThread && ServerThread->joinable()) {
        ServerThread->join();
    }
    delete ServerThread;

}

void ServerMain::StopServer() {
    SharedStatus = Status.lock();
    if (SharedStatus) {
        SharedStatus->store(false);
    }
    if (ServerThread && ServerThread->joinable()) {
        ServerThread->join();
    }
}

void ServerMain::RunServer() {
    SharedStatus = Status.lock();  // Lock the weak pointer to get a shared_ptr
    if (!SharedStatus) {
        cerr << "Status is not available. Cannot start server." << endl;
        return;
    }

    SharedStatus->store(true);
    Loop();
}

void ServerMain::Init() {
    FileDescriptor = -1;
    EpollFD = -1;
    ServerName = "";
    ServerThread = nullptr;
    weak_ptr<atomic<bool>> stat={};
    SharedStatus = make_shared<atomic<bool>>(false);
    Status = SharedStatus;
    m_Connections = make_shared<mutex>();
    m_Requests = make_shared<mutex>();
    EpollFD = epoll_create1(0);
    Connections = make_shared<vector<shared_ptr<Connection>>>();
    Requests = make_shared<vector<shared_ptr<Request>>>();
    if (EpollFD == -1) {
        cerr << "Error in epoll_create1:\n\t" << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }

    addrinfo hints{}, *server_inf, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int rv = getaddrinfo(nullptr, SERVER_PORT, &hints, &server_inf);
    if (rv) {
        cerr << "Error in getaddrinfo():\n\t" << gai_strerror(rv) << endl;
        exit(EXIT_FAILURE);
    }

    int p_errno = -1;
    for (p = server_inf; p != nullptr; p = p->ai_next) {
        FileDescriptor = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (FileDescriptor == -1) {
            perror("socket()");
            p_errno = errno;
            continue;
        }
        int yes = 1;
        if (setsockopt(FileDescriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            cerr << "Error in setsockopt():\n\t" << strerror(errno) << endl;
            exit(EXIT_FAILURE);
        }
        if (bind(FileDescriptor, p->ai_addr, p->ai_addrlen) == -1) {
            perror("bind()");
            p_errno = errno;
            continue;
        }
        break;
    }
    freeaddrinfo(server_inf);

    if (!p) {
        cerr << "Bind failure, cause:\n\t" << strerror(p_errno) << endl;
        exit(EXIT_FAILURE);
    }

    if (listen(FileDescriptor, 10) == -1) {
        cerr << "Listen failure, cause:\n\t" << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }

    int flags = fcntl(FileDescriptor, F_GETFL, 0);
    if (flags == -1) {
        cerr << "Error in fcntl:\n\t" << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }
    flags |= O_NONBLOCK;
    if (fcntl(FileDescriptor, F_SETFL, flags) == -1) {
        cerr << "Error in fcntl:\n\t" << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }

    epoll_event event{};
    event.data.fd = FileDescriptor;
    event.events = EPOLLIN;
    if (epoll_ctl(EpollFD, EPOLL_CTL_ADD, FileDescriptor, &event) == -1) {
        cerr << "Error in epoll_ctl:\n\t" << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }
}

void ServerMain::Loop() {
    ServerThread = new thread([this, weakStatus = Status]() -> void {
        vector<epoll_event> events(MAX_EVENTS);

        while (true) {
            SharedStatus = weakStatus.lock();  // Re-lock to check if it's still valid
            if (!SharedStatus || !SharedStatus->load()) {
                break;
            }

            int n = epoll_wait(EpollFD, events.data(), MAX_EVENTS, -1);
            if (errno == EINTR)
                continue;
            if (n == -1) {
                perror("epoll_wait");
                exit(EXIT_FAILURE);
            }

            for (int i = 0; i < n; ++i) {
                if (events[i].data.fd == FileDescriptor) {
                    sockaddr_storage addr{};
                    socklen_t addr_len = sizeof(addr);
                    int new_fd = accept(FileDescriptor, (sockaddr *) &addr, &addr_len);
                    if (new_fd == -1) {
                        perror("accept");
                        continue;
                    }

                    int flags = fcntl(new_fd, F_GETFL, 0);
                    if (flags == -1) {
                        perror("fcntl");
                        close(new_fd);
                        continue;
                    }
                    flags |= O_NONBLOCK;
                    if (fcntl(new_fd, F_SETFL, flags) == -1) {
                        perror("fcntl");
                        close(new_fd);
                        continue;
                    }

                    epoll_event event{};
                    event.data.fd = new_fd;
                    event.events = EPOLLIN | EPOLLET;
                    if (epoll_ctl(EpollFD, EPOLL_CTL_ADD, new_fd, &event) == -1) {
                        perror("epoll_ctl");
                        close(new_fd);
                        continue;
                    }

                    auto connection = make_shared<Connection>(new_fd, addr, true);
                    PushConnection(connection);
                } else {
                    auto connection = GetConnectionByFd(events[i].data.fd);
                    if (!connection) continue;

                    ssize_t bytes_read = connection->Read();
                    if (bytes_read <= 0) {
                        if (bytes_read == 0) {
                            cerr << "Received zero bytes, closing connection" << endl;
                            close(connection->FileDescriptor);
                            RemoveConnection(connection->FileDescriptor);
                        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            perror("Error in recv()");
                            close(connection->FileDescriptor);
                            RemoveConnection(connection->FileDescriptor);
                        }
                        continue;
                    } else {
                        printf("Request Received\n");
                        string buff = string(connection->ReadBuffer.begin(), connection->ReadBuffer.end());
                        auto curReq = make_shared<Request>(Request::Deserialize(buff));
                        PushRequest(curReq);
                    }
                }
            }
            Enact();
        }
    });
    ServerThread->detach();
}

void ServerMain::PushConnection(const shared_ptr<Connection>& connection) {
    {
        lock_guard<mutex> guard(*m_Connections);
        Connections->push_back(connection);
    }
}

void ServerMain::RemoveConnection(int fd) {
    {
        lock_guard<mutex> guard(*m_Connections);
        int i;
        for (i=0;i<Connections->size() && Connections->at(i)->FileDescriptor!=fd;i++);
        Connections->erase(Connections->begin() + i);
    }
}


shared_ptr<Connection> ServerMain::GetConnectionByFd(int fd) {
    {
        lock_guard<mutex> guard(*m_Connections);
        int i;
        for (i = 0; i < Connections->size() && Connections->at(i)->FileDescriptor != fd; i++);
        return Connections->at(i);
    }
}

void ServerMain::Enact() {
    function<bool()> isRequestsEmpty = [this]() -> bool {
        {
            lock_guard<mutex> guard(*m_Requests);
            return Requests->empty();
        }
    };
    while (!isRequestsEmpty()){
        auto cur_req = PopRequest();
        auto stream = ofstream(cur_req->MachineName + "-" +  cur_req->UserName + ".txt", ios::app);
        stream << "["
               << cur_req->MachineName
               << "]:["
               << cur_req->UserName
               << "]:["
               << cur_req->Timestamp
               << "]="
               << cur_req->Line
               << "\n";
        stream.close();
    }
}

void ServerMain::PushRequest(const shared_ptr<Request>& request) {
    {
        lock_guard<mutex> guard(*m_Requests);
        Requests->push_back(request);
    }
}

shared_ptr<Request> ServerMain::PopRequest() {
    {
        lock_guard<mutex> guard(*m_Requests);
        if (Requests->empty())
            return nullptr;
        auto ret = Requests->at(Requests->size()-1);
        Requests->pop_back();
        return ret;
    }
}
