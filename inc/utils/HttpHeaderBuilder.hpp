#pragma once

#include <string>
#include <map>

namespace HttpHeaderBuilder {
    // Focused on HTTP header construction only
    
    // Basic header setters (stateless functions)
    void setContentType(std::map<std::string, std::string>& headers, const std::string& type);
    void setContentLength(std::map<std::string, std::string>& headers, size_t length);
    void setCacheControl(std::map<std::string, std::string>& headers, const std::string& control);
    void setServer(std::map<std::string, std::string>& headers, const std::string& serverName = "webserv/1.0");
    void setDate(std::map<std::string, std::string>& headers);
    void setConnection(std::map<std::string, std::string>& headers, bool keepAlive);
    
    // Additional useful headers for HTTP compliance
    void setLocation(std::map<std::string, std::string>& headers, const std::string& location);
    void setLastModified(std::map<std::string, std::string>& headers, const std::string& datetime);
    void setExpires(std::map<std::string, std::string>& headers, const std::string& datetime);
    void setETag(std::map<std::string, std::string>& headers, const std::string& etag);
    void setTransferEncoding(std::map<std::string, std::string>& headers, const std::string& encoding = "chunked");
    
    // Common header patterns (return ready-to-use header maps)
    std::map<std::string, std::string> createHtmlHeaders(size_t contentLength);
    std::map<std::string, std::string> createPlainTextHeaders(size_t contentLength);
    std::map<std::string, std::string> createFileHeaders(const std::string& mimeType, size_t contentLength, bool cacheable = true);
    std::map<std::string, std::string> createErrorHeaders(size_t contentLength);
    std::map<std::string, std::string> createBasicHeaders(const std::string& contentType, size_t contentLength);
}
