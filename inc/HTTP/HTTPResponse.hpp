#pragma once
#include "HTTPTypes.hpp"
#include <map>
#include <memory>
#include <string>

class HTTPResponse {
private:
  HTTP::StatusCode _status;
  HTTP::Version _version;
  std::map<std::string, std::string> _headers;
  std::string _body;
  size_t _content_length;

public:
  HTTPResponse();
  void setStatus(HTTP::StatusCode status);
  void setVersion(HTTP::Version version);
  void setHeader(const std::string& key, const std::string& value);
  void setBody(const std::string& body);
  void setContentType(const std::string& type);
  void setContentLength(size_t length);
  HTTP::StatusCode getStatus() const;
  HTTP::Version getVersion() const;
  std::string getHeader(const std::string& key) const;
  std::map<std::string, std::string> getHeaders() const;
  std::string getBody() const;
  size_t getContentLength() const;
  std::string generateResponse() const;
};
