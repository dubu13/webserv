#include "HTTP/HTTP.hpp"
#include "utils/HttpUtils.hpp"
#include "utils/Logger.hpp"
#include <iostream>
#include <sstream>

namespace HTTP {

bool parseRequest(const std::string &data, Request &request) {
  Logger::debugf("HTTP::parseRequest - parsing %zu bytes of request data", data.size());
  
  try {
    size_t pos = data.find("\r\n");
    if (pos == std::string::npos) {
      Logger::debug("HTTP parse failed: no CRLF found in request line");
      return false;
    }
    
    std::string requestLineStr = data.substr(0, pos);
    Logger::debugf("HTTP request line: %s", requestLineStr.c_str());
    request.requestLine = parseRequestLine(requestLineStr);
    
    size_t headerStart = pos + 2;
    size_t headerEnd = data.find("\r\n\r\n", headerStart);
    if (headerEnd == std::string::npos) {
      Logger::debug("HTTP parse failed: no double CRLF found (incomplete headers)");
      return false;
    }
    
    std::string_view headersView = std::string_view(data).substr(headerStart, headerEnd - headerStart);
    Logger::debugf("HTTP headers section (%zu bytes)", headersView.size());
    HttpUtils::parseHeaders(headersView, request.headers);
    Logger::debugf("Parsed %zu HTTP headers", request.headers.size());
    
    size_t bodyStart = headerEnd + 4;
    if (bodyStart < data.length()) {
      request.body = data.substr(bodyStart);
      Logger::debugf("HTTP body: %zu bytes", request.body.size());
    } else {
      Logger::debug("HTTP request has no body");
    }
    
    auto connectionHeader = getHeader(request.headers, "Connection");
    request.keepAlive = (connectionHeader == "keep-alive" || 
                        (request.requestLine.version == "HTTP/1.1" && connectionHeader != "close"));
    Logger::debugf("HTTP keep-alive: %s", request.keepAlive ? "true" : "false");
    
    Logger::debug("HTTP request parsing completed successfully");
    return true;
  } catch (const std::exception &e) {
    Logger::errorf("HTTP parse exception: %s", e.what());
    return false;
  }
}

RequestLine parseRequestLine(const std::string &line) {
  RequestLine requestLine;
  
  std::string_view lineView(line);
  auto [method, rest] = HttpUtils::splitFirst(lineView, ' ');
  auto [uri, version] = HttpUtils::splitFirst(HttpUtils::trimWhitespace(rest), ' ');
  
  requestLine.method = stringToMethod(std::string(method));
  requestLine.uri = std::string(HttpUtils::trimWhitespace(uri));
  requestLine.version = std::string(HttpUtils::trimWhitespace(version));
  
  return requestLine;
}

std::string getHeader(const std::map<std::string, std::string> &headers, const std::string &key) {
  auto it = headers.find(key);
  return (it != headers.end()) ? it->second : "";
}

void setHeader(std::map<std::string, std::string> &headers, const std::string &key, const std::string &value) {
  headers[key] = value;
}

bool hasHeader(const std::map<std::string, std::string> &headers, const std::string &key) {
  return headers.find(key) != headers.end();
}

}
