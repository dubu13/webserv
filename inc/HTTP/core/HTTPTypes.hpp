#pragma once
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace HTTP {

enum class Method { GET, POST, DELETE, OPTIONS, UNKNOWN };

enum class StatusCode {
  OK = 200,
  CREATED = 201,
  NO_CONTENT = 204,
  MOVED_PERMANENTLY = 301,
  FOUND = 302,
  BAD_REQUEST = 400,
  UNAUTHORIZED = 401,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  METHOD_NOT_ALLOWED = 405,
  REQUEST_TIMEOUT = 408,
  CONFLICT = 409,
  PAYLOAD_TOO_LARGE = 413,
  URI_TOO_LONG = 414,
  INTERNAL_SERVER_ERROR = 500,
  NOT_IMPLEMENTED = 501
};

inline Method stringToMethod(const std::string &methodStr) {
  if (methodStr == "GET")
    return Method::GET;
  if (methodStr == "POST")
    return Method::POST;
  if (methodStr == "DELETE")
    return Method::DELETE;
  return Method::UNKNOWN;
}

inline std::string methodToString(Method method) {
  switch (method) {
  case Method::GET:
    return "GET";
  case Method::POST:
    return "POST";
  case Method::DELETE:
    return "DELETE";
  default:
    return "UNKNOWN";
  }
}

inline std::string statusToString(StatusCode status) {
  static const std::unordered_map<StatusCode, std::string_view>
      statusToStringMap = {
          {StatusCode::OK, "OK"},
          {StatusCode::CREATED, "Created"},
          {StatusCode::NO_CONTENT, "No Content"},
          {StatusCode::MOVED_PERMANENTLY, "Moved Permanently"},
          {StatusCode::FOUND, "Found"},
          {StatusCode::BAD_REQUEST, "Bad Request"},
          {StatusCode::UNAUTHORIZED, "Unauthorized"},
          {StatusCode::FORBIDDEN, "Forbidden"},
          {StatusCode::NOT_FOUND, "Not Found"},
          {StatusCode::METHOD_NOT_ALLOWED, "Method Not Allowed"},
          {StatusCode::CONFLICT, "Conflict"},
          {StatusCode::PAYLOAD_TOO_LARGE, "Payload Too Large"},
          {StatusCode::URI_TOO_LONG, "URI Too Long"},
          {StatusCode::INTERNAL_SERVER_ERROR, "Internal Server Error"},
          {StatusCode::NOT_IMPLEMENTED, "Not Implemented"},
          {StatusCode::REQUEST_TIMEOUT, "Request Timeout"}};
  auto it = statusToStringMap.find(status);
  if (it != statusToStringMap.end())
    return std::string(it->second);
  return "Unknown";
}

inline std::string statusToString(int statusCode) {
  return statusToString(static_cast<StatusCode>(statusCode));
}

} // namespace HTTP
