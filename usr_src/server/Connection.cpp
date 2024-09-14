#include "Connection.hpp"
#include <unistd.h>
#include <cstring>
#include <mutex>

#define BUFFER_SIZE 1024

int Connection::count = 0;

Connection::Connection() {

}

Connection::Connection(int fd, sockaddr_storage addr, bool guest)
        : FileDescriptor(fd), Address(addr), IsGuest(guest) {
    Setup();
}

Connection::~Connection() {
    WriteBuffer.clear();
    WriteBuffer.resize(0);
    ReadBuffer.clear();
    ReadBuffer.resize(0);
    close(FileDescriptor);
}

ssize_t Connection::Read() {
    lock_guard<std::mutex> guard(*ReadMutex);
    ssize_t bytesRead = 0;
    size_t totalBytesRead = 0;
    const int buffer_size = 1024;
    char tmp_r_buff[buffer_size] = {};
    memset(&tmp_r_buff, 0, sizeof tmp_r_buff);
    bytesRead = recv(FileDescriptor, &tmp_r_buff, buffer_size - totalBytesRead, 0);

    // Transfer data to ReadBuffer
    ReadBuffer.clear();
    for (int i = 0; i < buffer_size && tmp_r_buff[i] != '\0'; ++i) {
        ReadBuffer.push_back(tmp_r_buff[i]);
        totalBytesRead++;
    }
    return totalBytesRead;
}

ssize_t Connection::Write() {
    lock_guard<std::mutex> guard(*WriteMutex);
    ssize_t bytesWritten = write(FileDescriptor, WriteBuffer.data(), WriteBuffer.size());
    if (bytesWritten > 0)
        WriteBuffer.clear();
    return bytesWritten;
}

void Connection::Setup() {
    WriteMutex = make_shared<mutex>();
    ReadMutex = make_shared<mutex>();
    OwnerMutex = make_shared<mutex>();
    ReadBuffer.reserve(BUFFER_SIZE);
    WriteBuffer.reserve(BUFFER_SIZE);
    ID = count++;
}
