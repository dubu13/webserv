#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <map>

namespace HttpParser {
    // Pure HTTP parsing utilities - no state, no dependencies
    
    // String parsing utilities (high performance with string_view)
    std::string_view trimWhitespace(std::string_view str);
    std::pair<std::string_view, std::string_view> splitFirst(std::string_view str, char delimiter);
    std::string_view extractValue(std::string_view header_line);
    
    // Request line parsing
    std::pair<std::string_view, std::string_view> parseRequestLine(std::string_view line);
    
    // Header parsing utilities
    void parseHeaderLine(std::string_view line, std::map<std::string, std::string>& headers);
    void parseHeaders(std::string_view headerSection, std::map<std::string, std::string>& headers);
    
    // Validation utilities
    bool isValidHttpVersion(std::string_view version);
    bool isValidHttpMethod(std::string_view method);
    bool isValidUri(std::string_view uri);
}
