#include "HTTP/HTTPPostRequest.hpp"
#include "HTTP/IHTTPRequest.hpp"
#include <iostream>

HTTPPostRequest::HTTPPostRequest() = default;

HTTPPostRequest::~HTTPPostRequest() = default;

bool HTTPPostRequest::parseRequest(const std::string &data) {
  try {
    size_t pos = data.find("\r\n");
    if (pos == std::string::npos) {
      throw HTTPParseException("Invalid request format");
    }
    _requestLine = parseRequestLine(data.substr(0, pos));
    
    if (_requestLine.method != HTTP::Method::POST) {
      throw HTTPParseException("Invalid method: " + 
                             HTTP::methodToString(_requestLine.method));
    }

    size_t headerStart = pos + 2;
    size_t headerEnd = data.find("\r\n\r\n", headerStart);

    if (headerEnd == std::string::npos) {
      throw HTTPParseException("Invalid header format");
    }

    _headers = parseHeaders(data.substr(headerStart, headerEnd - headerStart));

    // Extract body - everything after the headers
    size_t bodyStart = headerEnd + 4; // Skip the \r\n\r\n
    if (bodyStart < data.length()) {
      _body = data.substr(bodyStart);
    }

    return true;
  } catch (const HTTPParseException &e) {
    std::cerr << "Error parsing request: " << e.what() << std::endl;
    return false;
  }
}

HTTP::Method HTTPPostRequest::getMethod() const {
  return _requestLine.method;
}

std::string HTTPPostRequest::getUri() const {
  return _requestLine.uri;
}

std::string HTTPPostRequest::getVersion() const {
  return _requestLine.version;
}

std::string HTTPPostRequest::getBody() const {
  return _body;
}

std::string HTTPPostRequest::getHeader(const std::string &key) const {
  auto it = _headers.find(key);
  if (it != _headers.end())
    return it->second;
  return "";
}

std::map<std::string, std::string> HTTPPostRequest::getHeaders() const {
  return _headers;
}