#include "utils/HttpUtils.hpp"
#include "utils/Logger.hpp"
#include <string>
#include <map>
#include <string_view>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace HttpUtils {

// Modern string_view implementations for better performance
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
    // Split method and rest
    auto [method, rest] = splitFirst(line, ' ');
    
    // Split URI and version
    auto [uri, version] = splitFirst(trimWhitespace(rest), ' ');
    
    return {trimWhitespace(uri), trimWhitespace(version)};
}

void parseHeaderLine(std::string_view line, std::map<std::string, std::string>& headers) {
    auto [key_view, value_view] = splitFirst(line, ':');
    if (value_view.empty()) {
        Logger::debug("HttpUtils::parseHeaderLine - Invalid header line (no colon found)");
        return;
    }
    
    std::string key = std::string(trimWhitespace(key_view));
    std::string value = std::string(trimWhitespace(value_view));
    
    headers[key] = value;
    Logger::debugf("HttpUtils::parseHeaderLine - Parsed header: %s: %s", key.c_str(), value.c_str());
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
        
        start = end + 2; // Skip \r\n
        if (start >= headerSection.length()) break;
    }
}

std::string buildHttpStatusLine(int statusCode, const std::string& statusText, 
                               const std::string& version) {
  Logger::debugf("HttpUtils::buildHttpStatusLine - Building status line: %s %d %s", 
                 version.c_str(), statusCode, statusText.c_str());
  
  std::string result;
  result.reserve(version.length() + 20 + statusText.length());
  result += version + " " + std::to_string(statusCode) + " " + statusText + "\r\n";
  return result;
}

size_t estimateResponseSize(const std::string& version, 
                           const std::map<std::string, std::string>& headers,
                           const std::string& body, bool keepAlive) {
  size_t estimated = version.length() + 50 +
                    headers.size() * 50 +
                    body.length() + 20 +
                    (keepAlive ? 40 : 20);
  
  Logger::debugf("HttpUtils::estimateResponseSize - Estimated response size: %zu bytes (version: %zu, headers: %zu*50, body: %zu)", 
                 estimated, version.length(), headers.size(), body.length());
  return estimated;
}

// HTTP header building utilities (stateless functions)

void setContentType(std::map<std::string, std::string>& headers, const std::string& type) {
    headers["Content-Type"] = type;
}

void setContentLength(std::map<std::string, std::string>& headers, size_t length) {
    headers["Content-Length"] = std::to_string(length);
}

void setCacheControl(std::map<std::string, std::string>& headers, const std::string& control) {
    headers["Cache-Control"] = control;
}

void setServer(std::map<std::string, std::string>& headers, const std::string& serverName) {
    headers["Server"] = serverName;
}

void setDate(std::map<std::string, std::string>& headers) {
    std::time_t now = std::time(nullptr);
    std::tm* gmt = std::gmtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(gmt, "%a, %d %b %Y %H:%M:%S GMT");
    headers["Date"] = oss.str();
}

// Common header patterns (return ready-to-use header maps)
std::map<std::string, std::string> createHtmlHeaders(size_t contentLength) {
    std::map<std::string, std::string> headers;
    setContentType(headers, "text/html");
    setContentLength(headers, contentLength);
    setServer(headers);
    setDate(headers);
    return headers;
}

std::map<std::string, std::string> createPlainTextHeaders(size_t contentLength) {
    std::map<std::string, std::string> headers;
    setContentType(headers, "text/plain");
    setContentLength(headers, contentLength);
    setServer(headers);
    setDate(headers);
    return headers;
}

std::map<std::string, std::string> createFileHeaders(const std::string& mimeType, size_t contentLength, bool cacheable) {
    std::map<std::string, std::string> headers;
    setContentType(headers, mimeType);
    setContentLength(headers, contentLength);
    setServer(headers);
    setDate(headers);
    
    if (cacheable) {
        setCacheControl(headers, "public, max-age=3600");
    }
    return headers;
}

std::map<std::string, std::string> createErrorHeaders(size_t contentLength) {
    std::map<std::string, std::string> headers;
    setContentType(headers, "text/html");
    setContentLength(headers, contentLength);
    setServer(headers);
    setDate(headers);
    setCacheControl(headers, "no-cache");
    return headers;
}

std::map<std::string, std::string> createBasicHeaders(const std::string& contentType, size_t contentLength) {
    std::map<std::string, std::string> headers;
    setContentType(headers, contentType);
    setContentLength(headers, contentLength);
    setServer(headers);
    setDate(headers);
    return headers;
}

}
