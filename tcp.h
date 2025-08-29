#ifndef TCP_H
#define TCP_H

#include <iostream>
#include <netinet/in.h>

class TCP {
public:

    TCP(const std::string& ip, int port);
    ~TCP();

    TCP(const TCP&) = delete;
    TCP& operator=(const TCP&) = delete;

    bool bindAndListen(int backlog);
    void serve(class IConnectionHandler& connectionHandler);

    ssize_t receive(int fd, void* buffer, size_t len);
    bool sendAll(int fd, const void* data, size_t len);

private:
    
    int  socketFileDescriptor;
    sockaddr_in address;
};

#endif
