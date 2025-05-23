#include "HTTP/HTTPDeleteRequest.hpp"
#include "HTTP/IHTTPRequest.hpp"
#include <iostream>

HTTPDeleteRequest::HTTPDeleteRequest() = default;

HTTPDeleteRequest::~HTTPDeleteRequest() = default;

bool HTTPDeleteRequest::parseRequest(const std::string &data) {
  try {
    size_t pos = data.find("\r\n");
    if (pos == std::string::npos) {
      throw HTTPParseException("Invalid request format");
    }
    _requestLine = parseRequestLine(data.substr(0, pos));
    
    if (_requestLine.method != HTTP::Method::DELETE) {
      throw HTTPParseException("Invalid method: " + 
                             HTTP::methodToString(_requestLine.method));
    }

    size_t headerStart = pos + 2;
    size_t headerEnd = data.find("\r\n\r\n", headerStart);

    if (headerEnd == std::string::npos) {
      throw HTTPParseException("Invalid header format");
    }

    _headers = parseHeaders(data.substr(headerStart, headerEnd - headerStart));

    return true;
  } catch (const HTTPParseException &e) {
    std::cerr << "Error parsing request: " << e.what() << std::endl;
    return false;
  }
}

HTTP::Method HTTPDeleteRequest::getMethod() const {
  return _requestLine.method;
}

std::string HTTPDeleteRequest::getUri() const {
  return _requestLine.uri;
}

std::string HTTPDeleteRequest::getVersion() const {
  return _requestLine.version;
}

std::string HTTPDeleteRequest::getBody() const {
  return ""; // DELETE requests typically don't have a body
}

std::string HTTPDeleteRequest::getHeader(const std::string &key) const {
  auto it = _headers.find(key);
  if (it != _headers.end())
    return it->second;
  return "";
}

std::map<std::string, std::string> HTTPDeleteRequest::getHeaders() const {
  return _headers;
}