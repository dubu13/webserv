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
            Logger::warn("HTTP request missing line terminator");
            return false;
        }
        
        std::string_view requestLineView = std::string_view(data).substr(0, pos);
        request.requestLine = parseRequestLine(requestLineView);
        
        size_t headerStart = pos + 2;
        size_t headerEnd = data.find("\r\n\r\n", headerStart);
        if (headerEnd == std::string::npos) {
            Logger::warn("HTTP request missing header terminator");
            return false;
        }
        
        std::string_view headersView = std::string_view(data).substr(headerStart, headerEnd - headerStart);
        parseHeaders(headersView, request.headers);
        
        size_t bodyStart = headerEnd + 4;
        if (bodyStart < data.length()) {
            request.body = data.substr(bodyStart);
        }
        
        // Validate the parsed request according to HTTP standards
        if (!validateHttpRequest(request)) {
            Logger::warn("HTTP request failed validation");
            return false;
        }
        
        auto connectionIt = request.headers.find("Connection");
        std::string connectionHeader = (connectionIt != request.headers.end()) ? connectionIt->second : "";
        request.keepAlive = (connectionHeader == "keep-alive" || 
                            (request.requestLine.version == "HTTP/1.1" && connectionHeader != "close"));
        
        return true;
    } catch (const std::exception& e) {
        Logger::error("HTTP parsing failed: " + std::string(e.what()));
        return false;
    }
}

RequestLine parseRequestLine(std::string_view line) {
    RequestLine requestLine;
    
    // Parse in-place using string_view to avoid unnecessary copies
    size_t pos = 0;
    size_t space1 = line.find(' ', pos);
    if (space1 == std::string_view::npos) {
        throw std::invalid_argument("Invalid request line: missing method delimiter");
    }
    
    std::string_view method = line.substr(pos, space1 - pos);
    pos = space1 + 1;
    
    size_t space2 = line.find(' ', pos);
    if (space2 == std::string_view::npos) {
        throw std::invalid_argument("Invalid request line: missing URI delimiter");
    }
    
    std::string_view uri = line.substr(pos, space2 - pos);
    pos = space2 + 1;
    
    std::string_view version = line.substr(pos);
    
    // Only convert to string when storing in final structure
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
    
    // Validate header against CRLF injection
    if (key.find('\r') != std::string::npos || key.find('\n') != std::string::npos ||
        value.find('\r') != std::string::npos || value.find('\n') != std::string::npos) {
        Logger::warn("HTTP header injection attempt detected, ignoring header: " + key);
        return;
    }
    
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

bool validateHttpRequest(const Request& request) {
    if (request.requestLine.version != "HTTP/1.1" && 
        request.requestLine.version != "HTTP/1.0") {
        Logger::warn("Unsupported HTTP version: " + request.requestLine.version);
        return false;
    }

    if (request.requestLine.uri.length() > 2048) {
        Logger::warn("URI too long: " + std::to_string(request.requestLine.uri.length()) + " bytes");
        return false;
    }

    if (request.requestLine.version == "HTTP/1.1") {
        auto host_it = request.headers.find("Host");
        if (host_it == request.headers.end() || host_it->second.empty()) {
            Logger::warn("HTTP/1.1 request missing required Host header");
            return false;
        }
    }

    auto cl_it = request.headers.find("Content-Length");
    if (cl_it != request.headers.end()) {
        try {
            size_t content_length = std::stoull(cl_it->second);
            constexpr size_t MAX_CONTENT_LENGTH = 10 * 1024 * 1024; // 10MB
            if (content_length > MAX_CONTENT_LENGTH) {
                Logger::warn("Content-Length too large: " + cl_it->second);
                return false;
            }
        } catch (const std::exception& e) {
            Logger::warn("Invalid Content-Length format: " + cl_it->second);
            return false;
        }
    }
    try {
        HTTP::methodToString(request.requestLine.method);
    } catch (const std::exception& e) {
        Logger::warn("Invalid HTTP method in request");
        return false;
    }
    
    return true;
}

} // namespace Parser
} // namespace HTTP
