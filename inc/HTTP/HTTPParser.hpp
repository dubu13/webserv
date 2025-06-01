#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <map>
#include "HTTP/HTTPTypes.ipp"

namespace HTTP {
    // HTTP Request Line structure
    struct RequestLine {
        Method method;
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
    
    // HTTP parsing utilities - no state, no dependencies
    namespace Parser {
        bool parseRequest(const std::string& data, Request& request);
        RequestLine parseRequestLine(std::string_view line);
        
        // String parsing utilities (high performance with string_view)
        std::string_view trimWhitespace(std::string_view str);
        std::pair<std::string_view, std::string_view> splitFirst(std::string_view str, char delimiter);
        
        // Header parsing utilities
        void parseHeaderLine(std::string_view line, std::map<std::string, std::string>& headers);
        void parseHeaders(std::string_view headerSection, std::map<std::string, std::string>& headers);
    }
    
} // namespace HTTP
