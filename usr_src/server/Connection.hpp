#include <string>
#include <vector>
#include <sys/socket.h>
#include <memory>
#include <mutex>
using namespace std;

#ifndef KEYLOGGER_CONNECTIONS_H
#define KEYLOGGER_CONNECTIONS_H


class Connection {
public:
    int ID;
    bool IsGuest;
    int FileDescriptor;
    sockaddr_storage Address;
    vector<char> ReadBuffer;
    vector<char> WriteBuffer;

    Connection();
    explicit Connection(int fd, sockaddr_storage addr, bool guest);

    ~Connection();

    ssize_t Read();

    ssize_t Write();
    static int count;
    shared_ptr<mutex> WriteMutex;
    shared_ptr<mutex> ReadMutex;
    shared_ptr<mutex> OwnerMutex;
    void Setup();
};


#endif //KEYLOGGER_CONNECTIONS_H
