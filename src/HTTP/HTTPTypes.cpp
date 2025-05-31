#include "HTTP/HTTPTypes.hpp"
#include "utils/Logger.hpp"
#include <stdexcept>
#include <unordered_map>
#include <string_view>
namespace HTTP {
static const std::unordered_map<std::string_view, Method> stringToMethodMap = {
    {"GET", Method::GET},
    {"POST", Method::POST},
    {"DELETE", Method::DELETE},
    {"HEAD", Method::HEAD},
    {"PUT", Method::PUT},
    {"PATCH", Method::PATCH}
};
static const std::unordered_map<Method, std::string_view> methodToStringMap = {
    {Method::GET, "GET"},
    {Method::POST, "POST"},
    {Method::DELETE, "DELETE"},
    {Method::HEAD, "HEAD"},
    {Method::PUT, "PUT"},
    {Method::PATCH, "PATCH"}
};
Method stringToMethod(const std::string &method) {
  Logger::debugf("HTTPTypes::stringToMethod - Converting method string: %s", method.c_str());
  auto it = stringToMethodMap.find(method);
  if (it != stringToMethodMap.end()) {
    Logger::debugf("Method '%s' mapped successfully", method.c_str());
    return it->second;
  }
  Logger::errorf("Invalid HTTP method: %s", method.c_str());
  throw std::runtime_error("Invalid HTTP method: " + method);
}
std::string methodToString(Method method) {
  auto it = methodToStringMap.find(method);
  if (it != methodToStringMap.end()) {
    return std::string(it->second);
  }
  return "UNKNOWN";
}
static const std::unordered_map<StatusCode, std::string_view> statusToStringMap = {
    {StatusCode::OK, "OK"},
    {StatusCode::CREATED, "Created"},
    {StatusCode::NO_CONTENT, "No Content"},
    {StatusCode::BAD_REQUEST, "Bad Request"},
    {StatusCode::FORBIDDEN, "Forbidden"},
    {StatusCode::NOT_FOUND, "Not Found"},
    {StatusCode::METHOD_NOT_ALLOWED, "Method Not Allowed"},
    {StatusCode::CONFLICT, "Conflict"},
    {StatusCode::PAYLOAD_TOO_LARGE, "Payload Too Large"},
    {StatusCode::INTERNAL_SERVER_ERROR, "Internal Server Error"},
    {StatusCode::NOT_IMPLEMENTED, "Not Implemented"}
};
std::string statusToString(StatusCode status) {
  auto it = statusToStringMap.find(status);
  if (it != statusToStringMap.end()) {
    return std::string(it->second);
  }
  return "Unknown";
}

// MIME type lookup - consolidated into HTTPTypes for efficiency
static const std::unordered_map<std::string_view, std::string_view> extensionToMimeMap = {
  {"html", "text/html"},
  {"htm", "text/html"},
  {"css", "text/css"},
  {"js", "text/javascript"},
  {"json", "application/json"},
  {"txt", "text/plain"},
  {"png", "image/png"},
  {"jpg", "image/jpeg"},
  {"jpeg", "image/jpeg"},
  {"gif", "image/gif"},
  {"pdf", "application/pdf"},
  {"ico", "image/x-icon"},
  {"svg", "image/svg+xml"},
  {"xml", "application/xml"},
  {"zip", "application/zip"},
  {"mp3", "audio/mpeg"},
  {"mp4", "video/mp4"}
};

std::string getMimeType(const std::string &path) {
  Logger::debugf("HTTPTypes::getMimeType - Determining MIME type for path: %s", path.c_str());
  
  size_t dotPos = path.find_last_of('.');
  if (dotPos == std::string::npos || dotPos == path.length() - 1) {
    Logger::debug("No file extension found, using default MIME type");
    return "application/octet-stream";
  }
  
  std::string ext = path.substr(dotPos + 1);
  Logger::debugf("File extension: %s", ext.c_str());
  
  // Convert to lowercase efficiently
  for (char &c : ext) {
    if (c >= 'A' && c <= 'Z') c += 32;
  }
  
  auto it = extensionToMimeMap.find(ext);
  std::string mimeType = (it != extensionToMimeMap.end()) ? std::string(it->second) : "application/octet-stream";
  Logger::debugf("MIME type resolved: %s", mimeType.c_str());
  return mimeType;
}

}
