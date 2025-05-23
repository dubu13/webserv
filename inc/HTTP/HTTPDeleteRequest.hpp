#pragma once

#include "IHTTPRequest.hpp"
#include <map>
#include <string>

class HTTPDeleteRequest : public IHTTPRequest {
private:
  HTTP::RequestLine _requestLine;
  std::map<std::string, std::string> _headers;

public:
  HTTPDeleteRequest();
  ~HTTPDeleteRequest() override;

  bool parseRequest(const std::string &data) override;
  HTTP::Method getMethod() const override;
  std::string getUri() const override;
  std::string getVersion() const override;
  std::string getBody() const override;
  std::string getHeader(const std::string &key) const override;
  std::map<std::string, std::string> getHeaders() const override;
};