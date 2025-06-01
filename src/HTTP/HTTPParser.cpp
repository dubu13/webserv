#include "HTTP/HTTPParser.hpp"
#include "utils/Logger.hpp"
#include <map>
#include <string_view>
#include <iostream>
#include <sstream>

namespace HTTP {

namespace Parser {

bool parseRequest(const std::string& data, Request& request) {
    try {
        size_t pos = data.find("\r\n");
        if (pos == std::string::npos) {
            return false;
        }
        
        std::string requestLineStr = data.substr(0, pos);
        request.requestLine = parseRequestLine(requestLineStr);
        
        size_t headerStart = pos + 2;
        size_t headerEnd = data.find("\r\n\r\n", headerStart);
        if (headerEnd == std::string::npos) {
            return false;
        }
        
        std::string_view headersView = std::string_view(data).substr(headerStart, headerEnd - headerStart);
        parseHeaders(headersView, request.headers);
        
        size_t bodyStart = headerEnd + 4;
        if (bodyStart < data.length()) {
            request.body = data.substr(bodyStart);
        }
        
        auto connectionIt = request.headers.find("Connection");
        std::string connectionHeader = (connectionIt != request.headers.end()) ? connectionIt->second : "";
        request.keepAlive = (connectionHeader == "keep-alive" || 
                            (request.requestLine.version == "HTTP/1.1" && connectionHeader != "close"));
        
        return true;
    } catch (const std::exception& e) {
                return false;
    }
}

RequestLine parseRequestLine(const std::string& line) {
    RequestLine requestLine;
    
    std::string_view lineView(line);
    auto [method, rest] = splitFirst(lineView, ' ');
    auto [uri, version] = splitFirst(trimWhitespace(rest), ' ');
    
    requestLine.method = HTTP::stringToMethod(std::string(method));
    requestLine.uri = std::string(trimWhitespace(uri));
    requestLine.version = std::string(trimWhitespace(version));
    
    return requestLine;
}

std::string_view trimWhitespace(std::string_view str) {
    const char* whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string_view::npos) return {};
    
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::pair<std::string_view, std::string_view> splitFirst(std::string_view str, char delimiter) {
    size_t pos = str.find(delimiter);
    if (pos == std::string_view::npos) {
        return {str, {}};
    }
    return {str.substr(0, pos), str.substr(pos + 1)};
}

void parseHeaderLine(std::string_view line, std::map<std::string, std::string>& headers) {
    auto [key_view, value_view] = splitFirst(line, ':');
    if (value_view.empty()) {
        return;
    }
    
    std::string key = std::string(trimWhitespace(key_view));
    std::string value = std::string(trimWhitespace(value_view));
    
    headers[key] = value;
}

void parseHeaders(std::string_view headerSection, std::map<std::string, std::string>& headers) {
    size_t start = 0;
    while (start < headerSection.length()) {
        size_t end = headerSection.find("\r\n", start);
        if (end == std::string_view::npos) {
            end = headerSection.length();
        }
        
        if (end > start) {
            std::string_view line = headerSection.substr(start, end - start);
            parseHeaderLine(line, headers);
        }
        
        start = end + 2;
        if (start >= headerSection.length()) break;
    }
}

} // namespace Parser
} // namespace HTTP
