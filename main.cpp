#include "tcp.h"
#include "connection.h"

#include <iostream>
#include <cstring>  // for std::strlen

class EchoHandler : public IConnectionHandler {
public:
    void handle(int fd, TCP& io) override {
        char buf[1024];

        for (;;) {
            ssize_t n = io.receive(fd, buf, sizeof(buf));
            if (n > 0) {
                // Echo back exactly what was received
                if (!io.sendAll(fd, buf, static_cast<size_t>(n))) {
                    std::cerr << "sendAll failed\n";
                    break;
                }
            } else if (n == 0) {
                // Client closed the connection
                break;
            } else {
                perror("recv");
                break;
            }
        }
    }
};


int main() {
    TCP server("127.0.0.1", 8888);
    server.bindAndListen();

    EchoHandler handler;
    server.serve(handler);
    
    return 0;
}
