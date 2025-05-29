#include "HTTPResponse.hpp"
#include <sstream>
HTTPResponse::HTTPResponse()
    : _status(HTTP::StatusCode::OK), _version(HTTP::Version::HTTP_1_1),
      _content_length(0) {}
void HTTPResponse::setStatus(HTTP::StatusCode status) {
  _status = status;
}
void HTTPResponse::setVersion(HTTP::Version version) {
  _version = version;
}
void HTTPResponse::setHeader(const std::string& key, const std::string& value) {
  _headers[key] = value;
}
void HTTPResponse::setBody(const std::string& body) {
  _body = body;
  _content_length = _body.size();
  setHeader("Content-Length", std::to_string(_content_length));
}
void HTTPResponse::setContentType(const std::string& type) {
  setHeader("Content-Type", type);
}
void HTTPResponse::setContentLength(size_t length) {
  _content_length = length;
  setHeader("Content-Length", std::to_string(_content_length));
}
HTTP::StatusCode HTTPResponse::getStatus() const {
  return _status;
}
HTTP::Version HTTPResponse::getVersion() const {
  return _version;
}
std::string HTTPResponse::getHeader(const std::string& key) const {
  auto it = _headers.find(key);
  if (it != _headers.end()) {
    return it->second;
  }
  return "";
}
std::map<std::string, std::string> HTTPResponse::getHeaders() const {
  return _headers;
}
std::string HTTPResponse::getBody() const {
  return _body;
}
size_t HTTPResponse::getContentLength() const {
  return _content_length;
}
std::string HTTPResponse::generateResponse() const {
  std::stringstream response;
  response << HTTP::versionToString(_version) << " "
           << static_cast<int>(_status) << " " << HTTP::statusToString(_status)
           << "\r\n";
  for (const auto &header : _headers) {
    response << header.first << ": " << header.second << "\r\n";
  }
  response << "\r\n";
  response << _body;
  return response.str();
}
