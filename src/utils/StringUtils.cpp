#include "utils/StringUtils.hpp"
#include "utils/Logger.hpp"
#include <string>
#include <map>

namespace StringUtils {

// Fast pointer-based parsing helpers
const char* findNextSpace(const char* start, const char* end) {
  while (start < end && *start != ' ') ++start;
  return start;
}

const char* skipWhitespace(const char* start, const char* end) {
  while (start < end && *start == ' ') ++start;
  return start;
}

const char* findLineEnd(const char* start, const char* end) {
  while (start < end && *start != '\r' && *start != '\n') ++start;
  return start;
}

const char* skipLineTerminators(const char* start, const char* end) {
  if (start < end && *start == '\r') ++start;
  if (start < end && *start == '\n') ++start;
  return start;
}

const char* findHeaderColon(const char* start, const char* end) {
  while (start < end && *start != ':') ++start;
  return start;
}

const char* trimTrailingSpaces(const char* start, const char* end) {
  while (end > start && *(end-1) == ' ') --end;
  return end;
}

std::string extractToken(const char* start, const char* end) {
  return std::string(start, end - start);
}

void parseHeaderLine(const char* lineStart, const char* lineEnd, 
                    std::map<std::string, std::string>& headers) {
  const char* colonPos = findHeaderColon(lineStart, lineEnd);
  
  if (colonPos < lineEnd) {
    const char* keyEnd = trimTrailingSpaces(lineStart, colonPos);
    std::string key = extractToken(lineStart, keyEnd);
    
    const char* valueStart = skipWhitespace(colonPos + 1, lineEnd);
    const char* valueEnd = trimTrailingSpaces(valueStart, lineEnd);
    std::string value = extractToken(valueStart, valueEnd);
    
    headers[key] = value;
    Logger::debugf("StringUtils::parseHeaderLine - Parsed header: %s: %s", key.c_str(), value.c_str());
  } else {
    Logger::debug("StringUtils::parseHeaderLine - Invalid header line (no colon found)");
  }
}

std::string buildHttpStatusLine(int statusCode, const std::string& statusText, 
                               const std::string& version) {
  Logger::debugf("StringUtils::buildHttpStatusLine - Building status line: %s %d %s", 
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
  
  Logger::debugf("StringUtils::estimateResponseSize - Estimated response size: %zu bytes (version: %zu, headers: %zu*50, body: %zu)", 
                 estimated, version.length(), headers.size(), body.length());
  return estimated;
}

}
