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

std::string buildResponse(StatusCode status, const std::map<std::string, std::string> &headers, 
                         const std::string &body, bool keepAlive) {
  Logger::debugf("HTTP::buildResponse - Building response with status %d, %zu headers, %zu body bytes, keepAlive=%s", 
                 static_cast<int>(status), headers.size(), body.size(), keepAlive ? "true" : "false");
  
  std::string response;
  response.reserve(256 + body.length() + headers.size() * 50);
  
  response += "HTTP/1.1 " + std::to_string(static_cast<int>(status)) + " " + statusToString(status) + "\r\n";
  
  if (keepAlive) {
    response += "Connection: keep-alive\r\nKeep-Alive: timeout=5, max=100\r\n";
  } else {
    response += "Connection: close\r\n";
  }
  
  for (const auto& header : headers) {
    if (header.first != "Connection" && header.first != "Keep-Alive") {
      response += header.first + ": " + header.second + "\r\n";
      Logger::debugf("HTTP response header: %s: %s", header.first.c_str(), header.second.c_str());
    }
  }
  
  response += "\r\n" + body;
  Logger::debugf("HTTP response built successfully: %zu total bytes", response.size());
  return response;
}

std::string createSimpleResponse(StatusCode status, const std::string &message) {
  Logger::debugf("HTTP::createSimpleResponse - Creating simple response with status %d and message length %zu", 
                 static_cast<int>(status), message.size());
  
  auto headers = HttpUtils::createPlainTextHeaders(message.size());
  return buildResponse(status, headers, message);
}

std::string createErrorResponse(StatusCode status, const std::string &errorMessage) {
  Logger::debugf("HTTP::createErrorResponse - Creating error response with status %d", static_cast<int>(status));
  
  std::string body = errorMessage;
  if (body.empty()) {
    std::string statusStr = std::to_string(static_cast<int>(status));
    std::string statusText = statusToString(status);
    body = "<html><body><h1>" + statusStr + " " + statusText + "</h1></body></html>";
    Logger::debugf("Generated default error body: %zu bytes", body.size());
  } else {
    Logger::debugf("Using provided error message: %zu bytes", body.size());
  }
  
  auto headers = HttpUtils::createErrorHeaders(body.size());
  return buildResponse(status, headers, body);
}

std::string createFileResponse(StatusCode status, const std::string &content, const std::string &contentType) {
  Logger::debugf("HTTP::createFileResponse - Creating file response with status %d, content-type '%s', content length %zu", 
                 static_cast<int>(status), contentType.c_str(), content.size());
  
  bool cacheable = (status == StatusCode::OK);
  auto headers = HttpUtils::createFileHeaders(contentType, content.size(), cacheable);
  return buildResponse(status, headers, content);
}

}
