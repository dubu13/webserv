#include "HTTPResponse.hpp"
#include <sstream>
#include <string_view>

HTTPResponse::HTTPResponse() noexcept
    : _status(HTTP::StatusCode::OK), _version(HTTP::Version::HTTP_1_1),
      _content_length(0) {}

void HTTPResponse::setStatus(HTTP::StatusCode status) noexcept {
  _status = status;
}

void HTTPResponse::setVersion(HTTP::Version version) noexcept {
  _version = version;
}

void HTTPResponse::setHeader(std::string_view key, std::string_view value) {
  _headers[std::string(key)] = std::string(value);
}

void HTTPResponse::setBody(std::string_view body) {
  _body = std::string(body);
  _content_length = _body.size();
  setHeader("Content-Length", std::to_string(_content_length));
}

void HTTPResponse::setContentType(std::string_view type) {
  setHeader("Content-Type", type);
}

void HTTPResponse::setContentLength(size_t length) noexcept {
  _content_length = length;
  setHeader("Content-Length", std::to_string(_content_length));
}

HTTP::StatusCode HTTPResponse::getStatus() const noexcept { return _status; }

HTTP::Version HTTPResponse::getVersion() const noexcept { return _version; }

std::optional<std::string_view>
HTTPResponse::getHeader(std::string_view key) const noexcept {
  auto it = _headers.find(std::string(key));
  if (it != _headers.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::map<std::string, std::string> HTTPResponse::getHeaders() const noexcept {
  return _headers;
}

std::string_view HTTPResponse::getBody() const noexcept { return _body; }

size_t HTTPResponse::getContentLength() const noexcept {
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

std::unique_ptr<HTTPResponse> HTTPResponse::createResponse() noexcept {
  return std::make_unique<HTTPResponse>();
}
