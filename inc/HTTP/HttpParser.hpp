#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <map>
#include "HTTP/HTTPTypes.hpp"

namespace HttpParser {
    // Pure HTTP parsing utilities - no state, no dependencies
    
    // HTTP Request Line structure
    struct RequestLine {
      HTTP::Method method;
      std::string uri;
      std::string version;
    };
    
    // HTTP Request structure
    struct Request {
      RequestLine requestLine;
      std::map<std::string, std::string> headers;
      std::string body;
      bool keepAlive = false;
      Request() = default;
    };
    
    // High-level parsing functions
    bool parseRequest(const std::string &data, Request &request);
    RequestLine parseRequestLine(const std::string &line);
    
    // String parsing utilities (high performance with string_view)
    std::string_view trimWhitespace(std::string_view str);
    std::pair<std::string_view, std::string_view> splitFirst(std::string_view str, char delimiter);
    
    // Header parsing utilities
    void parseHeaderLine(std::string_view line, std::map<std::string, std::string>& headers);
    void parseHeaders(std::string_view headerSection, std::map<std::string, std::string>& headers);
}
