#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <cstdlib>

class TcpSocket {
    public:
        TcpSocket(const char* ip, int port) {
            address.sin_family = AF_INET;
            address.sin_port = htons(port);

            if (!inet_aton(ip, &address.sin_addr)) {
                std::cerr << "Failed to assign ip address to the socket.\n";
                exit(1);
            }
            
            this->serverSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (this->serverSocket == -1) {
                std::cerr << "Failed to create a socket.\n";
                exit(1);
            }
        }

        ~TcpSocket() {
            close(this->serverSocket);
        }

        void bindSocket() {
            bind(serverSocket, (struct sockaddr*)&address, sizeof(address));
            if (listen(serverSocket, 5) == -1) {
                std::cerr << "Failed to listen.\n";
            }

            // INET_ADDRSTRLEN = 16
            char ipv4_address[INET_ADDRSTRLEN];
            std::cout << "The server is listening on " << inet_ntop(AF_INET, &address.sin_addr, ipv4_address, INET_ADDRSTRLEN) << ":" << ntohs(address.sin_port) << "\n";
        }

        void startServer() {
            std::cout << "the server should've started here. This is a placeholder.\n";
        }

    private:
        sockaddr_in address {};
        int serverSocket;

};