#include "ServerConnection.h"

#include <sys/socket.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <atomic>
#include <memory>
#include <sys/time.h>
#include <thread>
#include <unistd.h>
#include <mutex>

ServerConnection::ServerConnection() {
    Setup();
}

ServerConnection::~ServerConnection() {
    Stop();

    if (t_Sender && t_Sender->joinable())
        t_Sender->join();
    delete t_Sender;

    while(!OutgoingRequests.empty())
        OutgoingRequests.pop();
}

ServerConnection::ServerConnection(string addr) :
HostAddr(move(addr)){
    Setup();
}

void ServerConnection::Start() {
    f_Stop->store(false);
    t_Sender = new thread([this]() {
        while (!f_Stop->load()) {
            auto cur = PopReq();
            if (cur== nullptr)
                continue;

            string serialized = cur->Serialzie();
            const char *buf = serialized.c_str();
            ssize_t sent_bytes = send(FDConnection, buf, serialized.size(), 0);

            if (sent_bytes == -1) {
                perror("send");
            }
        }
    });
}

void ServerConnection::Stop() {
    f_Stop->store(true);
    if (FDConnection == -1){
        shutdown(FDConnection, SHUT_RDWR);
        close(FDConnection);
        FDConnection = -1;
    }
}

void ServerConnection::MakeRequest(const shared_ptr<Request>& req) {
    PushReq(req);
}

void ServerConnection::Setup() {
    t_Sender = nullptr;

    f_Stop = make_shared<atomic<bool>>(false);
    OutgoingRequests = {};
    m_OutgoingRequests = make_shared<mutex>();
    isLoggedIn={};

    FDConnection = -1;
    if (HostAddr.empty())
        HostAddr = "localhost";
    addrinfo hints{}, *res = nullptr, *p = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int stat = getaddrinfo(HostAddr.c_str(), SERVER_PORT, &hints, &res);
    if (stat != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(stat));
        return;
    }

    for (p = res; p; p = p->ai_next) {
        FDConnection = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (FDConnection == -1) {
            perror("socket");
            continue;
        }
        if (connect(FDConnection, p->ai_addr, p->ai_addrlen) == -1) {
            perror("connect");
            close(FDConnection);
            FDConnection = -1;
            continue;
        }
        break;
    }

    if (FDConnection == -1) {
        freeaddrinfo(res);
        return;
    }

    freeaddrinfo(res);
}

void ServerConnection::PushReq(const shared_ptr<Request> &to_push) {
    {
        lock_guard<mutex> guard(*m_OutgoingRequests);
        OutgoingRequests.push(to_push);
    }
}

shared_ptr<Request> ServerConnection::PopReq() {
    {
        lock_guard<mutex> guard(*m_OutgoingRequests);
        if(OutgoingRequests.empty())
            return nullptr;
        auto tmp = OutgoingRequests.front();
        OutgoingRequests.pop();
        return tmp;
    }
}
