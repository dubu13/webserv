#include "HTTP/HTTPGetRequest.hpp"
#include "HTTP/IHTTPRequest.hpp"
#include <iostream>

HTTPGetRequest::HTTPGetRequest() = default;

HTTPGetRequest::~HTTPGetRequest() = default;

bool HTTPGetRequest::parseRequest(const std::string &data) {
  try {
    size_t pos = data.find("\r\n");
    if (pos == std::string::npos) {
      throw HTTPParseException("Invalid request format");
    }
    _requestLine = parseRequestLine(data.substr(0, pos));
    
    if (_requestLine.method != HTTP::Method::GET) {
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

HTTP::Method HTTPGetRequest::getMethod() const {
  return _requestLine.method;
}

std::string HTTPGetRequest::getUri() const { return _requestLine.uri; }

std::string HTTPGetRequest::getVersion() const { return _requestLine.version; }

std::string HTTPGetRequest::getBody() const { return ""; }

std::string HTTPGetRequest::getHeader(const std::string &key) const {
  auto it = _headers.find(key);
  if (it != _headers.end()) {
    return it->second;
  }
  return "";
}

std::map<std::string, std::string> HTTPGetRequest::getHeaders() const {
    return _headers;
}
