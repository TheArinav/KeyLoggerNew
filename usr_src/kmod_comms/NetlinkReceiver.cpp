#include "NetlinkReceiver.hpp"
#include <fcntl.h>  // For fcntl
#include <thread>
#include <chrono>


NetlinkReceiver::NetlinkReceiver(int protocol, int bufferSize)
        : sock_fd_(-1), receiving_(false), bufferSize_(bufferSize) {
    // Create a netlink socket
    sock_fd_ = socket(PF_NETLINK, SOCK_RAW, protocol);
    if (sock_fd_ < 0) {
        throw std::runtime_error("Failed to create netlink socket");
    }

    // Initialize the source address structure
    memset(&src_addr_, 0, sizeof(src_addr_));
    src_addr_.nl_family = AF_NETLINK;
    src_addr_.nl_pid = getpid();  // Unique PID for the user-space application

    if (bind(sock_fd_, (struct sockaddr*)&src_addr_, sizeof(src_addr_)) < 0) {
        close(sock_fd_);
        throw std::runtime_error("Failed to bind netlink socket");
    }

    // Initialize the destination address structure
    memset(&dest_addr_, 0, sizeof(dest_addr_));
    dest_addr_.nl_family = AF_NETLINK;
    dest_addr_.nl_pid = 0;  // Kernel's PID
    dest_addr_.nl_groups = 0;  // Unicast

    // Send an initial message to set the PID in the kernel
    struct nlmsghdr *nlh = static_cast<nlmsghdr *>(malloc(NLMSG_SPACE(bufferSize_)));
    if (!nlh) {
        throw std::runtime_error("Failed to allocate memory for nlmsghdr");
    }

    memset(nlh, 0, NLMSG_SPACE(bufferSize_));
    nlh->nlmsg_len = NLMSG_SPACE(bufferSize_);
    nlh->nlmsg_pid = getpid();  // PID of sending process
    nlh->nlmsg_flags = 0;  // No special flags

    struct iovec iov = { nlh, NLMSG_SPACE(bufferSize_) };
    struct msghdr msg = { (void *)&dest_addr_, sizeof(dest_addr_), &iov, 1, nullptr, 0, 0 };

    // Sending an initial message to kernel
    const char *initial_msg = "Hello, kernel";
    char *payload = (char *)((char *)nlh + NLMSG_HDRLEN);
    strncpy(payload, initial_msg, strlen(initial_msg) + 1);


    if (sendmsg(sock_fd_, &msg, 0) < 0) {
        perror("Failed to send initial message to kernel");
    }

    free(nlh);
}


NetlinkReceiver::~NetlinkReceiver() {
    if (sock_fd_ >= 0) {
        close(sock_fd_);
    }
}

void NetlinkReceiver::startReceiving() {
    receiving_ = true;
    while (receiving_) {
        receiveMessage();
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep briefly to avoid busy-waiting
    }
}

void NetlinkReceiver::stopReceiving() {
    receiving_ = false;
}

void NetlinkReceiver::receiveMessage() {
    auto *nlh = static_cast<nlmsghdr *>(malloc(NLMSG_SPACE(bufferSize_)));
    if (!nlh) {
        throw std::runtime_error("Failed to allocate memory for nlmsghdr");
    }

    memset(nlh, 0, NLMSG_SPACE(bufferSize_));
    struct iovec iov = { nlh, NLMSG_SPACE(bufferSize_) };
    struct msghdr msg = { (void *)&dest_addr_, sizeof(dest_addr_), &iov, 1, nullptr, 0, 0 };

    // Receive the message from the kernel
    ssize_t received_bytes = recvmsg(sock_fd_, &msg, 0);
    if (received_bytes < 0 && errno != EAGAIN) { // EAGAIN is expected if no data is available in non-blocking mode
        std::cerr << "Failed to receive message from kernel" << std::endl;
        free(nlh);
        return;
    }

    if (received_bytes > 0) {
        // Process the received message
        int keycode;
        char ascii_char;
        memcpy(&keycode, NLMSG_DATA(nlh), sizeof(int));
        memcpy(&ascii_char, static_cast<char*>(NLMSG_DATA(nlh)) + sizeof(int), sizeof(char));

        std::cout << "Received keycode: " << keycode << ", ASCII: " << ascii_char << std::endl;
    }

    free(nlh);
}
