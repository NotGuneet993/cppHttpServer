#include <iostream>
#include "http.h"
#include "tcp.h"

std::string Http::buildResponse(int status, std::string_view reason, std::string_view contentType, std::string_view body) {
    std::string h = "HTTP/1.1 " + std::to_string(status) + " " + std::string(reason) + "\r\n";
    if (!contentType.empty()) h += "Content-Type: " + std::string(contentType) + "\r\n";
    h += "Content-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n";

    return h + std::string(body);
}

void Http::sendBadRequest(int fd, TCP& io) {
    const std::string badRequestBody = "<h1>400 bad request</h1>";
    const std::string response = buildResponse(400, "Bad Request", "text/html; charset=utf-8", badRequestBody);
    io.sendAll(fd, response.data(), response.size());
}

Http::RequestLine Http::parseRequestLine(const std::string& message) {
    Http::RequestLine rl;

    auto crlfLocation = message.find("\r\n");
    if (crlfLocation == std::string::npos) return rl;

    std::string_view firstLine(message.data(), crlfLocation);

    size_t space1 = firstLine.find(' ');
    if (space1 == std::string::npos) return rl;

    size_t space2 = firstLine.find(' ', space1 + 1);
    if (space2 == std::string::npos) return rl;

    rl.method = std::string(firstLine.substr(0, space1));
    rl.target = std::string(firstLine.substr(space1 + 1, space2 - space1 - 1));
    rl.version = std::string(firstLine.substr(space2 + 1));

    std::cout << rl.method << " " << rl.target << " " << rl.version << "\n";
    rl.valid = true;
    return rl;
}

void Http::handleGet(int fd, TCP& io, std::string& target) {
    int counter {};
    std::string response;
    std::string body;

    if (target == "/") {
        ++counter;
        body = "<html><h1>visits: " + std::to_string(counter) + "</h1></html>";
        response = buildResponse(200, "OK", "text/html; charset=utf-8", body);
    } else if (target == "/favicon.ico") {
        response = buildResponse(204, "No Content", "text/html; charset=utf-8", "");
    } else {
        body = "<html><h1>404 Not Found</h1></html>";
        response = buildResponse(404, "Not Found", "text/html; charset=utf-8", body);
    }

    io.sendAll(fd, response.data(), response.size());
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
    
    Http::RequestLine rl = parseRequestLine(message);
    if (!rl.valid) {
        Http::sendBadRequest(fd, io);
        return;
    }

    if (rl.version != "HTTP/1.1" && rl.version != "HTTP/1.0") {
        Http::sendBadRequest(fd, io);
        return;
    }

    if (rl.method == "GET") handleGet(fd, io, rl.target);
    else if (rl.method == "POST") std::cout << "implementing...\n";
    else if (rl.method == "PUT") std::cout << "implementing...\n";
    else if (rl.method == "DELETE") std::cout << "implementing...\n";
    else {
        const std::string body = "<h1>405 Method Not Allowed</h1>";
        std::string resp = buildResponse(405, "Method Not Allowed", "text/html; charset=utf-8", body);
        io.sendAll(fd, resp.data(), resp.size());
    }

    return;
}