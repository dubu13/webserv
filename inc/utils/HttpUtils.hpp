#pragma once

#include <string>
#include <map>
#include <string_view>

namespace HttpUtils {
  // Modern string_view parsing utilities (high performance, safe)
  std::string_view trimWhitespace(std::string_view str);
  std::pair<std::string_view, std::string_view> splitFirst(std::string_view str, char delimiter);
  std::string_view extractValue(std::string_view header_line);
  std::pair<std::string_view, std::string_view> parseRequestLine(std::string_view line);
  
  // HTTP header parsing (modern approach)
  void parseHeaderLine(std::string_view line, std::map<std::string, std::string>& headers);
  void parseHeaders(std::string_view headerSection, std::map<std::string, std::string>& headers);
  
  // String building utilities
  std::string buildHttpStatusLine(int statusCode, const std::string& statusText, 
                                 const std::string& version = "HTTP/1.1");
  size_t estimateResponseSize(const std::string& version, 
                             const std::map<std::string, std::string>& headers,
                             const std::string& body, bool keepAlive = false);

  // HTTP header building utilities (stateless functions)
  
  // Basic header setters
  void setContentType(std::map<std::string, std::string>& headers, const std::string& type);
  void setContentLength(std::map<std::string, std::string>& headers, size_t length);
  void setCacheControl(std::map<std::string, std::string>& headers, const std::string& control);
  void setServer(std::map<std::string, std::string>& headers, const std::string& serverName = "webserv/1.0");
  void setDate(std::map<std::string, std::string>& headers);
  
  // Common header patterns (return ready-to-use header maps)
  std::map<std::string, std::string> createHtmlHeaders(size_t contentLength);
  std::map<std::string, std::string> createPlainTextHeaders(size_t contentLength);
  std::map<std::string, std::string> createFileHeaders(const std::string& mimeType, size_t contentLength, bool cacheable = true);
  std::map<std::string, std::string> createErrorHeaders(size_t contentLength);
  std::map<std::string, std::string> createBasicHeaders(const std::string& contentType, size_t contentLength);

}
