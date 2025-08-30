#ifndef HTTP_H
#define HTTP_H

#include <unordered_map>
#include <string>
#include <string_view>
#include "connection.h"

class Http : public IConnectionHandler {
public:

    void handle(int fd, TCP& io) override;
    
    struct RequestLine {
        std::string method;
        std::string target;
        std::string version;
        bool valid {false};
    };

private:

    std::unordered_map<std::string, int> stocks;

    std::string buildResponse(int status, std::string_view reason, std::string_view contentType, std::string_view body);
    void sendBadRequest(int fd, TCP& io);

    RequestLine parseRequestLine(const std::string& message);
    std::string renderPage(const std::unordered_map<std::string, int>& map) const;

    void handleGet(int fd, TCP& io, std::string& target);

};

#endif
