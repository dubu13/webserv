#pragma once
#include "HTTPTypes.hpp"
#include <map>
#include <string>

class HTTPRequest {
private:
  HTTP::RequestLine _requestLine;
  std::map<std::string, std::string> _headers;
  std::string _body;
  
  HTTP::RequestLine parseRequestLine(const std::string &line);
  std::map<std::string, std::string> parseHeaders(const std::string &headerSection);

public:
  HTTPRequest();
  ~HTTPRequest();
  
  bool parseRequest(const std::string &data);
  HTTP::Method getMethod() const;
  std::string getUri() const;
  std::string getVersion() const;
  std::string getBody() const;
  std::string getHeader(const std::string &key) const;
  std::map<std::string, std::string> getHeaders() const;
};
