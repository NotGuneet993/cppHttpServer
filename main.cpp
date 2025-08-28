#include <iostream>

// refactor to add to the OOP file
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>

int main() {
    
    // response string 
    std::string responseStr = "HTTP/1.1 200 OK\r\n\r\n <html>hello</html>";
    const char *pageResponse = responseStr.c_str();

    // create the socket. 
    // AF_INET is IPv4 & SOCK_STREAM is TCP
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == -1) {
        std::cerr << "Failed to create a socket.\n";
        return 1;
    }

    // create the listening address. 
    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(8888);

    if (!inet_aton("127.0.0.1", &address.sin_addr)) {
        std::cerr << "Failed to assign ip address to the socket.\n";
        return 1;
    }

    // bind the socket to the IP & port 
    bind(serverSocket, (struct sockaddr*)&address, sizeof(address));
    
    if (listen(serverSocket, 5) == -1) {
        std::cerr << "Failed to listen.\n";
    }

    // confirm the IP and port for the listener. 
    // INET_ADDRSTRLEN = 16
    char ipv4_address[INET_ADDRSTRLEN];
    std::cout << "The server is listening on " << inet_ntop(AF_INET, &address.sin_addr, ipv4_address, INET_ADDRSTRLEN) << ":" << ntohs(address.sin_port) << "\n";

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == -1) {
            std::cerr << "Failed to accept the message.\n";
        }

        // recieving data
        char buffer[1024] = { 0 };
        recv(clientSocket, buffer, sizeof(buffer), 0);
        std::cout << "Message from client: " << buffer << "\n";

        // send data back 
        send(clientSocket, pageResponse, responseStr.size(), 0);
        close(clientSocket);
    }

    close(serverSocket);

    return 0;
}