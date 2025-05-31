#pragma once
#include <string>

namespace HTTP {
  // HTTP Method enumeration
  enum class Method { GET, POST, DELETE, HEAD, PUT, PATCH };
  
  // HTTP Status Code enumeration
  enum class StatusCode {
    OK = 200,
    CREATED = 201,
    NO_CONTENT = 204,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    BAD_REQUEST = 400,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    CONFLICT = 409,
    PAYLOAD_TOO_LARGE = 413,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501
  };
  
  // Core HTTP utility functions
  Method stringToMethod(const std::string &method);
  std::string methodToString(Method method);
  std::string statusToString(StatusCode status);
  std::string getMimeType(const std::string &path);
}
