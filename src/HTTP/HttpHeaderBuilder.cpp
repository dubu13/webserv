#include "HTTP/HttpHeaderBuilder.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace HttpHeaderBuilder {

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
    std::stringstream oss;
    oss << std::put_time(std::gmtime(&now), "%a, %d %b %Y %H:%M:%S GMT");
    headers["Date"] = oss.str();
}

void setConnection(std::map<std::string, std::string>& headers, bool keepAlive) {
    if (keepAlive) {
        headers["Connection"] = "keep-alive";
        headers["Keep-Alive"] = "timeout=5, max=100";
    } else {
        headers["Connection"] = "close";
    }
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
    } else {
        setCacheControl(headers, "no-cache");
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

}
