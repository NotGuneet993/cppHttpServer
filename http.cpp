#include <iostream>
#include <random>
#include <iomanip>
#include <sstream>
#include "http.h"
#include "tcp.h"

std::string Http::buildResponse(int status, std::string_view reason, std::string_view contentType, std::string_view body, std::string_view otherHeaders) {
    std::string h = "HTTP/1.1 " + std::to_string(status) + " " + std::string(reason) + "\r\n";
    if (!otherHeaders.empty()) h += std::string(otherHeaders);
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

std::string Http::renderPage(const std::unordered_map<std::string, int>& map) const {
    std::string html = 
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"utf-8\">\n"
        "  <title>Stock holdings</title>\n"
        "</head>\n"
        "<body>\n"
        "  <h1>Stock holdings</h1>\n"
        "    <h2>Update holdings</h2>\n"
        "    <form action=\"/holdings\" method=\"post\">\n"
        "    <label>Ticker:\n"
        "        <input name=\"ticker\" pattern=\"[A-Za-z]{1,10}\" required>\n"
        "    </label>\n"
        "    <label>Quantity:\n"
        "        <input name=\"qty\" type=\"number\" step=\"1\" required>\n"
        "    </label>\n"
        "    <button type=\"submit\">Save</button>\n"
        "    </form>\n";

    if (map.empty()) {
        html += "  <p>You don’t own any stock.</p>\n";
    } else {
        html += 
            "  <table>\n"
            "    <tr><th>Ticker</th><th>Quantity</th></tr>\n";
        
        for (auto& [ticker, count] : map) {
            html += "    <tr><td>" + ticker + "</td><td>" + std::to_string(count) + "</td></tr>\n";
        }
        html += "  </table>\n";
    }   

    html +=
        "</body>\n"
        "</html>\n";

    return html;
}

bool Http::hasSession(const std::string& sid) {
    std::lock_guard<std::mutex> lock{this->sessionsMutex};
    return this->sessionsIds.find(sid) != this->sessionsIds.end();
}

std::string Http::createSessionId() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    uint64_t p1 = gen(), p2 = gen();

    std::ostringstream os;
    os << std::hex << std::setw(16) << std::setfill('0') << p1 << std::setw(16) << std::setfill('0') << p2;
    return os.str();
}

std::string Http::getCookieSid(std::string_view header) {
    auto find_cookie = [](std::string_view lgStr, std::string_view smStr)->size_t {
        for (size_t i = 0; i + smStr.size() <= lgStr.size(); ++i) {
            bool ok = true;
            for (size_t j = 0; j < smStr.size(); ++j) {
                if (std::tolower((unsigned char)lgStr[i+j]) != std::tolower((unsigned char)smStr[j])) {
                    ok = false;
                    break;
                }
            }
            if (ok) return i;
        }
        return std::string::npos;
    };

    size_t p = find_cookie(header, "Cookie:");
    if (p == std::string::npos) return {};
    size_t eol = header.find("\r\n", p);
    if (eol == std::string::npos) eol = header.size();
    std::string line(header.substr(p+7, eol - (p+7)));

    auto k = line.find("sid=");
    if (k == std::string::npos) return {};
    k += 4;
    size_t end = line.find(';', k);
    return (end == std::string::npos) ? line.substr(k) : line.substr(k, end - k);
}

std::unordered_map<std::string, int>& Http::createCookieSession(const std::string& sid) {
    std::lock_guard<std::mutex> lock{this->sessionsMutex};
    return this->sessionsIds[sid];
}

void Http::handleGet(int fd, TCP& io, std::string& target, std::unordered_map<std::string,int>& holdings, bool setCookie, const std::string& sid) {
    std::string response, body, extra;

    if (setCookie) {
        extra = "Set-Cookie: sid=" + sid + "; Path=/; HttpOnly\r\n";
    }

    if (target == "/") {
        body = renderPage(holdings);
        response = buildResponse(200, "OK", "text/html; charset=utf-8", body, extra);
    } else if (target == "/favicon.ico") {
        response = buildResponse(204, "No Content", "", "", extra);
    } else {
        body = "<html><h1>404 Not Found</h1></html>";
        response = buildResponse(404, "Not Found", "text/html; charset=utf-8", body, extra);
    }

    io.sendAll(fd, response.data(), response.size());
}

void Http::handlePost(int fd, TCP& io, std::string& target, std::string& message, std::unordered_map<std::string,int>& holdings, bool setCookie, const std::string& sid) {
    if (target != "/holdings") {
        auto body = "<html><h1>404 Not Found</h1></html>";
        auto response = buildResponse(404, "Not Found", "text/html; charset=utf-8", body);
        io.sendAll(fd, response.data(), response.size());
        return; 
    }

    const size_t headerEnd = message.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        sendBadRequest(fd, io);
        return;
    }
    std::string_view head(message.data(), headerEnd);
    const size_t body_start = headerEnd + 4;

    auto find_ci = [](std::string_view msgStr, std::string_view sub) -> size_t {
        for (size_t i = 0; i + sub.size() <= msgStr.size(); ++i) {
            bool eq = true;
            for (size_t j = 0; j < sub.size(); ++j) {
                unsigned char a = static_cast<unsigned char>(msgStr[i + j]);
                unsigned char b = static_cast<unsigned char>(sub[j]);
                if (std::tolower(a) != std::tolower(b)) { eq = false; break; }
            }
            if (eq) return i;
        }
        return std::string::npos;
    };

    size_t bodyLen = 0;
    size_t pos = find_ci(head, "Content-Length:");
    if (pos != std::string::npos) {
        pos += 15;
        while (pos < head.size() && (head[pos] == ' ' || head[pos] == '\t')) ++pos;
        size_t eol = head.find("\r\n", pos);
        if (eol == std::string::npos) eol = head.size();
        size_t end = eol;
        while (end > pos && (head[end - 1] == ' ' || head[end - 1] == '\t')) --end;
        std::string num(head.substr(pos, end - pos));
        bodyLen = static_cast<size_t>(std::strtoul(num.c_str(), nullptr, 10));
    }

    std::string msgBody = message.substr(body_start);
    while (msgBody.size() < bodyLen) {
        char buf[4096];
        ssize_t n = io.receive(fd, buf, sizeof buf);
        if (n <= 0) break;
        msgBody.append(buf, static_cast<size_t>(n));
    }

    std::cout << "Body (" << msgBody.size() << "/" << bodyLen << "): [" << msgBody << "]\n";

    auto parseForm = [](std::string_view s){
        std::unordered_map<std::string,std::string> kv;
        size_t i = 0;
        while (i <= s.size()) {
            size_t amp = s.find('&', i);
            if (amp == std::string::npos) amp = s.size();
            auto pair = s.substr(i, amp - i);
            size_t eq = pair.find('=');
            if (eq != std::string::npos)
                kv[std::string(pair.substr(0, eq))] = std::string(pair.substr(eq + 1));
            i = amp + 1;
        }
        return kv;
    };

    auto form = parseForm(msgBody);
    std::string ticker = form["ticker"];
    int qtyChange = std::atoi(form["qty"].c_str());

    for (auto& c : ticker) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    {
        std::lock_guard<std::mutex> lock(this->sessionsMutex);
        int current = holdings[ticker];
        int updated = current + qtyChange;
        if (updated <= 0) holdings.erase(ticker);
        else holdings[ticker] = updated;
    }

    std::string extra;
    if (setCookie) extra = "Set-Cookie: sid=" + sid + "; Path=/; HttpOnly\r\n";

    const std::string response =
        "HTTP/1.1 303 See Other\r\n"
        "Location: /\r\n" + extra +
        "Content-Length: 0\r\n"
        "Connection: close\r\n\r\n";
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

    size_t headerEnd = message.find("\r\n\r\n");
    std::string_view head(message.data(), headerEnd);

    std::string sid = getCookieSid(head);
    bool newSession = sid.empty() || !hasSession(sid);
    if (newSession) sid = createSessionId();
    auto& holdings = createCookieSession(sid);

    if (rl.method == "GET") handleGet(fd, io, rl.target, holdings, newSession, sid);
    else if (rl.method == "POST") handlePost(fd, io, rl.target, message, holdings, newSession, sid);
    else {
        const std::string body = "<h1>405 Method Not Allowed</h1>";
        std::string resp = buildResponse(405, "Method Not Allowed", "text/html; charset=utf-8", body);
        io.sendAll(fd, resp.data(), resp.size());
    }

    return;
}
