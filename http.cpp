#include <iostream>

#include "http.h"
#include "tcp.h"

void Http::sendBadRequest(int fd, TCP& io) {
    const std::string badRequestBody = "<h1>400 bad request</h1>";
    const std::string badRequestHeader =
    "HTTP/1.1 400 Bad Request\r\n"
    "Content-Type: text/html; charset=utf-8\r\n"
    "Content-Length: " + std::to_string(badRequestBody.size()) + "\r\n"
    "Connection: close\r\n\r\n";

    const std::string response = badRequestHeader + badRequestBody;
    io.sendAll(fd, response.c_str(), response.size());
}

void Http::handle(int fd, TCP& io) {
    constexpr size_t MAX_HEADER_SIZE = 16 * 1024;       // 16kb
    constexpr size_t MAX_CHUNK_SIZE = 4 * 1024;         // 4kb

    std::string message;
    message.reserve(MAX_HEADER_SIZE);
    char buffer[MAX_CHUNK_SIZE];

    while (true) {
        ssize_t n = io.receive(fd, buffer, sizeof(buffer));

        if (n > 0) {
            message.append(buffer, n);

            if (message.size() >= MAX_HEADER_SIZE) {
                Http::sendBadRequest(fd, io);
                return;
            }

            if (message.find("\r\n\r\n") != std::string::npos) break;

        } else if (n == 0) {
            return;
        } else {
            perror("Message extraction");
            std::cerr << "There was a problem extracting the received data.";
            return;
        }
    }
    
    // the header is valid so start branching. 
    auto crlfLocation = message.find("\r\n");
    if (crlfLocation == std::string::npos) {
        Http::sendBadRequest(fd, io);
        return;
    }

    std::string_view firstLine(message.data(), crlfLocation);

    size_t space1 = firstLine.find(' ');
    if (space1 == std::string::npos)  Http::sendBadRequest(fd, io);

    size_t space2 = firstLine.find(' ', space1 + 1);
    if (space2 == std::string::npos)  Http::sendBadRequest(fd, io);

    std::string method(firstLine.substr(0, space1));
    std::string target(firstLine.substr(space1 + 1, space2 - space1 - 1));
    std::string version(firstLine.substr(space2 + 1));

}