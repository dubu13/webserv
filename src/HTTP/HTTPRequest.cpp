#include "HTTP/HTTPRequest.hpp"
#include <iostream>

HTTPRequest::HTTPRequest() = default;
HTTPRequest::~HTTPRequest() = default;

bool HTTPRequest::parseRequest(const std::string &data) {
  try {
    size_t pos = data.find("\r\n");
    if (pos == std::string::npos) {
      std::cerr << "Invalid request format" << std::endl;
      return false;
    }
    _requestLine = parseRequestLine(data.substr(0, pos));
    
    size_t headerStart = pos + 2;
    size_t headerEnd = data.find("\r\n\r\n", headerStart);
    if (headerEnd == std::string::npos) {
      std::cerr << "Invalid header format" << std::endl;
      return false;
    }
    _headers = parseHeaders(data.substr(headerStart, headerEnd - headerStart));
    
    size_t bodyStart = headerEnd + 4;
    if (bodyStart < data.length()) {
      _body = data.substr(bodyStart);
    }
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Error parsing request: " << e.what() << std::endl;
    return false;
  }
}

HTTP::Method HTTPRequest::getMethod() const {
  return _requestLine.method;
}

std::string HTTPRequest::getUri() const {
  return _requestLine.uri;
}

std::string HTTPRequest::getVersion() const {
  return _requestLine.version;
}

std::string HTTPRequest::getBody() const {
  return _body;
}

std::string HTTPRequest::getHeader(const std::string &key) const {
  auto it = _headers.find(key);
  if (it != _headers.end())
    return it->second;
  return "";
}

std::map<std::string, std::string> HTTPRequest::getHeaders() const {
  return _headers;
}

HTTP::RequestLine HTTPRequest::parseRequestLine(const std::string &line) {
  HTTP::RequestLine requestLine;
  std::string::size_type start = 0;
  std::string::size_type end = line.find(' ');
  if (end == std::string::npos) {
    throw std::runtime_error("Invalid request line format");
  }
  requestLine.method = HTTP::stringToMethod(line.substr(start, end - start));
  start = end + 1;
  end = line.find(' ', start);
  if (end == std::string::npos) {
    throw std::runtime_error("Invalid request line format");
  }
  requestLine.uri = line.substr(start, end - start);
  start = end + 1;
  requestLine.version = line.substr(start);
  return requestLine;
}

std::map<std::string, std::string> HTTPRequest::parseHeaders(const std::string &headerSection) {
  std::map<std::string, std::string> headers;
  std::string::size_type start = 0;
  std::string::size_type end = 0;
  while ((end = headerSection.find("\r\n", start)) != std::string::npos) {
    std::string line = headerSection.substr(start, end - start);
    if (line.empty()) {
      break;
    }
    std::string::size_type colonPos = line.find(':');
    if (colonPos != std::string::npos) {
      std::string key = line.substr(0, colonPos);
      std::string value = line.substr(colonPos + 1);
      if (!value.empty() && value[0] == ' ') {
        value = value.substr(1);
      }
      headers[key] = value;
    }
    start = end + 2;
  }
  return headers;
}
