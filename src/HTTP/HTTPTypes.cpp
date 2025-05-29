#include "HTTP/HTTPTypes.hpp"
#include <stdexcept>
namespace HTTP {
Method stringToMethod(const std::string &method) {
  if (method == "GET")
    return Method::GET;
  if (method == "POST")
    return Method::POST;
  if (method == "DELETE")
    return Method::DELETE;
  if (method == "HEAD")
    return Method::HEAD;
  if (method == "PUT")
    return Method::PUT;
  if (method == "PATCH")
    return Method::PATCH;
  throw std::runtime_error("Invalid HTTP method: " + method);
}
std::string methodToString(Method method) {
  switch (method) {
  case Method::GET:
    return "GET";
  case Method::POST:
    return "POST";
  case Method::DELETE:
    return "DELETE";
  case Method::HEAD:
    return "HEAD";
  case Method::PUT:
    return "PUT";
  case Method::PATCH:
    return "PATCH";
  default:
    return "UNKNOWN";
  }
}
std::string statusToString(StatusCode status) {
  switch (status) {
  case StatusCode::OK:
    return "OK";
  case StatusCode::CREATED:
    return "Created";
  case StatusCode::NO_CONTENT:
    return "No Content";
  case StatusCode::BAD_REQUEST:
    return "Bad Request";
  case StatusCode::FORBIDDEN:
    return "Forbidden";
  case StatusCode::NOT_FOUND:
    return "Not Found";
  case StatusCode::METHOD_NOT_ALLOWED:
    return "Method Not Allowed";
  case StatusCode::CONFLICT:
    return "Conflict";
  case StatusCode::INTERNAL_SERVER_ERROR:
    return "Internal Server Error";
  case StatusCode::NOT_IMPLEMENTED:
    return "Not Implemented";
  default:
    return "Unknown";
  }
}
std::string versionToString(Version version) {
  switch (version) {
  case Version::HTTP_1_0:
    return "HTTP/1.0";
  case Version::HTTP_1_1:
    return "HTTP/1.1";
  default:
    return "HTTP/1.1";
  }
}
} // namespace HTTP
