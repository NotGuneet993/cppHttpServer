#include <iostream>
#include "tcp.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <stdexcept>

TCP::TCP(const std::string& ip, int port) {
    this->address.sin_family = AF_INET;
    this->address.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &this->address.sin_addr) != 1) {
        std::cerr << "Failed to assign ip address to the socket.\n";
        throw std::runtime_error("Failed to assign IP address to the socket\n");
    }

    this->socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (this->socketFileDescriptor == -1) {
        throw std::runtime_error("Failed to create the socket.\n");
    }
}

TCP::~TCP() {
    close(this->socketFileDescriptor);
}