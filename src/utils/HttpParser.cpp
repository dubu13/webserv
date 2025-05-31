#include "utils/HttpParser.hpp"
#include "utils/Logger.hpp"
#include <map>

namespace HttpParser {

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

std::string_view extractValue(std::string_view header_line) {
    auto [key, value] = splitFirst(header_line, ':');
    return trimWhitespace(value);
}

std::pair<std::string_view, std::string_view> parseRequestLine(std::string_view line) {
    auto [method, rest] = splitFirst(line, ' ');
    auto [uri, version] = splitFirst(trimWhitespace(rest), ' ');
    
    return {trimWhitespace(uri), trimWhitespace(version)};
}

void parseHeaderLine(std::string_view line, std::map<std::string, std::string>& headers) {
    auto [key_view, value_view] = splitFirst(line, ':');
    if (value_view.empty()) {
        Logger::debug("HttpParser::parseHeaderLine - Invalid header line (no colon found)");
        return;
    }
    
    std::string key = std::string(trimWhitespace(key_view));
    std::string value = std::string(trimWhitespace(value_view));
    
    headers[key] = value;
    Logger::debugf("HttpParser::parseHeaderLine - Parsed header: %s: %s", key.c_str(), value.c_str());
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

bool isValidHttpVersion(std::string_view version) {
    return version == "HTTP/1.0" || version == "HTTP/1.1";
}

bool isValidHttpMethod(std::string_view method) {
    return method == "GET" || method == "POST" || method == "DELETE" || 
           method == "HEAD" || method == "PUT" || method == "PATCH";
}

bool isValidUri(std::string_view uri) {
    return !uri.empty() && uri[0] == '/';
}

}
