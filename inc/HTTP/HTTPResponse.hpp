#pragma once

#include "HTTPTypes.hpp"
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

class HTTPResponse {
private:
  HTTP::StatusCode _status;
  HTTP::Version _version;
  std::map<std::string, std::string> _headers;
  std::string _body;
  size_t _content_length;

public:
  HTTPResponse() noexcept;
  // Setters
  void setStatus(HTTP::StatusCode status) noexcept;
  void setVersion(HTTP::Version version) noexcept;
  void setHeader(std::string_view key, std::string_view value);
  void setBody(std::string_view body);
  void setContentType(std::string_view type);
  void setContentLength(size_t length) noexcept;
  // Getters
  HTTP::StatusCode getStatus() const noexcept;
  HTTP::Version getVersion() const noexcept;
  std::optional<std::string_view>
  getHeader(std::string_view key) const noexcept;
  std::map<std::string, std::string> getHeaders() const noexcept;
  std::string_view getBody() const noexcept;
  size_t getContentLength() const noexcept;

  std::string generateResponse() const;
  static std::unique_ptr<HTTPResponse> createResponse() noexcept;
};
