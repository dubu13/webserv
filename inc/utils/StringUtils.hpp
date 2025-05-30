#pragma once

#include <string>
#include <map>

namespace StringUtils {
  // HTTP string parsing utilities
  const char* findNextSpace(const char* start, const char* end);
  const char* skipWhitespace(const char* start, const char* end);
  const char* findLineEnd(const char* start, const char* end);
  const char* skipLineTerminators(const char* start, const char* end);
  const char* findHeaderColon(const char* start, const char* end);
  const char* trimTrailingSpaces(const char* start, const char* end);
  std::string extractToken(const char* start, const char* end);
  
  // HTTP header parsing
  void parseHeaderLine(const char* lineStart, const char* lineEnd, 
                      std::map<std::string, std::string>& headers);
  
  // String building utilities
  std::string buildHttpStatusLine(int statusCode, const std::string& statusText, 
                                 const std::string& version = "HTTP/1.1");
  size_t estimateResponseSize(const std::string& version, 
                             const std::map<std::string, std::string>& headers,
                             const std::string& body, bool keepAlive = false);
}
