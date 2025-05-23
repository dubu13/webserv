#pragma once

#include "HTTPTypes.hpp"
#include <map>
#include <stdexcept>
#include <string>
#include <memory>
#include <iostream>

class HTTPParseException : public std::runtime_error {
public:
  explicit HTTPParseException(const std::string &message);
};

class IHTTPRequest {
public:
  virtual ~IHTTPRequest() = default;

  // pure virtual methods
  virtual bool parseRequest(const std::string &data) = 0;
  virtual HTTP::Method getMethod() const = 0;
  virtual std::string getUri() const = 0;
  virtual std::string getVersion() const = 0;
  virtual std::string getBody() const = 0;
  virtual std::string getHeader(const std::string &key) const = 0;
  virtual std::map<std::string, std::string> getHeaders() const = 0;

  // Factory method
  static std::unique_ptr<IHTTPRequest>
  createRequest(const std::string &requestData);

protected:
  static HTTP::RequestLine parseRequestLine(const std::string &line);
  static std::map<std::string, std::string>
  parseHeaders(const std::string &headerSection);
};