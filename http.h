#ifndef HTTP_H
#define HTTP_H

#include <unordered_map>
#include <string>
#include <string_view>
#include "connection.h"
#include <mutex>

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

    std::unordered_map<std::string, std::unordered_map<std::string, int>> sessionsIds;
    std::mutex sessionsMutex;

    std::string buildResponse(int status, std::string_view reason, std::string_view contentType, std::string_view body, std::string_view otherHeaders = "");
    void sendBadRequest(int fd, TCP& io);

    RequestLine parseRequestLine(const std::string& message);
    std::string renderPage(const std::unordered_map<std::string, int>& map) const;

    void handleGet(int fd, TCP& io, std::string& target, std::unordered_map<std::string,int>& holdings, bool setCookie, const std::string& sid);
    void handlePost(int fd, TCP& io, std::string& target, std::string& message, std::unordered_map<std::string,int>& holdings, bool setCookie, const std::string& sid);

    bool hasSession(const std::string& sid);
    static std::string createSessionId();
    static std::string getCookieSid(std::string_view header);
    std::unordered_map<std::string, int>& createCookieSession(const std::string& sid);

};

#endif
