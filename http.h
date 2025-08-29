#ifndef HTTP_H
#define HTTP_H

#include <unordered_map>
#include <string>
#include <string_view>
#include "connection.h"

class Http : public IConnectionHandler {
public:

    void handle(int fd, TCP& io) override;

private:

    int counter;
    std::unordered_map<std::string, int> colors;

    bool readHeaders(int fd, TCP&io, std::string& bytes, size_t maxHeaderBytes);
    bool parseRequestLine(const std::string& head, std::string& method, std::string& target, std::string& version);
    std::string buildResponse(int status, std::string_view reason, std::string_view contentType, std::string_view body, bool close);

};

#endif
