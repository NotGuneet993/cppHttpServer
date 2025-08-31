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

void Http::handleGet(int fd, TCP& io, std::string& target) {
    std::string response;
    std::string body;

    if (target == "/") {
        body = renderPage(this->stocks);
        response = buildResponse(200, "OK", "text/html; charset=utf-8", body);
    } else if (target == "/favicon.ico") {
        response = buildResponse(204, "No Content", "", "");
    } else {
        body = "<html><h1>404 Not Found</h1></html>";
        response = buildResponse(404, "Not Found", "text/html; charset=utf-8", body);
    }

    io.sendAll(fd, response.data(), response.size());
}

void Http::handlePost(int fd, TCP& io, std::string& target, std::string& message) {
    std::string response;
    std::string body;

    if (target != "/holdings") {
        body = "<html><h1>404 Not Found</h1></html>";
        response = buildResponse(404, "Not Found", "text/html; charset=utf-8", body);
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

    // --- case-insensitive find helper ---
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

    int current = stocks[ticker];
    int updated = current + qtyChange;

    if (updated <= 0)
        stocks.erase(ticker);
    else
        stocks[ticker] = updated;

    response =
        "HTTP/1.1 303 See Other\r\n"
        "Location: /\r\n"
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

    if (rl.method == "GET") handleGet(fd, io, rl.target);
    else if (rl.method == "POST") handlePost(fd, io, rl.target, message);
    else {
        const std::string body = "<h1>405 Method Not Allowed</h1>";
        std::string resp = buildResponse(405, "Method Not Allowed", "text/html; charset=utf-8", body);
        io.sendAll(fd, resp.data(), resp.size());
    }

    return;
}
