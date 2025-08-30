#include <iostream>
#include "tcp.h"
#include "connection.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <stdexcept>

TCP::TCP(const std::string& ip, int port) {
    this->address.sin_family = AF_INET;
    this->address.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &this->address.sin_addr) != 1) {
        perror("inet_pton");
        throw std::runtime_error("Failed to assign IP address to the socket\n");
    }

    this->socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->socketFd == -1) {
        perror("socket");
        throw std::runtime_error("Failed to create the socket.\n");
    }
}

TCP::~TCP() {
    close(this->socketFd);
}

bool TCP::bindAndListen(int backlog) {
    int option {1};
    if (setsockopt(this->socketFd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1) {
        perror("setsockopt");
        throw std::runtime_error("Failed to re-use the socket.\n");
    }

    if (bind(this->socketFd, (struct sockaddr*)&this->address, sizeof(this->address)) == -1) {
        perror("bind");
        throw std::runtime_error("Failed to bind to the socket.\n");
    }

    if (listen(this->socketFd, backlog) == -1) {
        perror("listen");
        throw std::runtime_error("Failed to listen.");
    }

    char ipv4_address[INET_ADDRSTRLEN];
    std::cout << "The server is listening on " << inet_ntop(AF_INET, &this->address.sin_addr, ipv4_address, INET_ADDRSTRLEN) << ":" << ntohs(this->address.sin_port) << "\n";

    return true; 
}

void TCP::serve(IConnectionHandler& handler) {
    while (true) {
        int clientSocket = accept(this->socketFd, nullptr, nullptr);
        if (clientSocket == -1) {
            perror("accept");
            throw std::runtime_error("Failed to accept the message.");
        }

        handler.handle(clientSocket, *this);
        close(clientSocket);
    } 
}

ssize_t TCP::receive(int fd, void* buffer, size_t len) {
    while (true) {
        ssize_t n = recv(fd, buffer, len, 0);
        if (n >= 0) return n;
        if (errno == EINTR) continue;

        return -1;
    }
}

bool TCP::sendAll(int fd, const void* data, size_t len) {
    const char* p = static_cast<const char*>(data);
    size_t sent = 0;

    while (sent < len) {
        ssize_t n = send(fd, p + sent, len - sent, 0);

        if (n > 0) {
            sent += static_cast<size_t>(n);
        } else if( n == -1 && errno == EINTR) {
            continue;
        } else {
            return false;
        }
    }

    return true;
}
