#ifndef NETLINK_RECEIVER_HPP
#define NETLINK_RECEIVER_HPP

#include <sys/socket.h>
#include <netlink/netlink.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <atomic>

#include "../client/ServerConnection.h"


class NetlinkReceiver {
public:
    NetlinkReceiver(int protocol, int bufferSize = 4096);
    ~NetlinkReceiver();

    void startReceiving();
    void stopReceiving();

private:
    ServerConnection SC;
    void receiveMessage();

    int sock_fd_;
    sockaddr_nl src_addr_;
    sockaddr_nl dest_addr_;
    atomic<bool> receiving_;
    int bufferSize_;
};

#endif // NETLINK_RECEIVER_HPP
