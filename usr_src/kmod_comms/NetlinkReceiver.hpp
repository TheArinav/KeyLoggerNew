#ifndef NETLINK_RECEIVER_HPP
#define NETLINK_RECEIVER_HPP

#include <sys/socket.h>   // For socket functions
#include <netlink/netlink.h>  // If using the libnl library
#include <unistd.h>        // For close()
#include <stdexcept>       // For exceptions
#include <cstring>         // For memset
#include <iostream>        // For std::cout

class NetlinkReceiver {
public:
    NetlinkReceiver(int protocol, int bufferSize = 4096);
    ~NetlinkReceiver();

    void startReceiving();
    void stopReceiving();

private:
    void receiveMessage();

    int sock_fd_;
    struct sockaddr_nl src_addr_;
    struct sockaddr_nl dest_addr_;
    bool receiving_;
    int bufferSize_;
};

#endif // NETLINK_RECEIVER_HPP
