#include "HTTP/HTTP.hpp"
#include "utils/StringUtils.hpp"
#include <iostream>
#include <sstream>

namespace HTTP {

bool parseRequest(const std::string &data, Request &request) {
  try {
    size_t pos = data.find("\r\n");
    if (pos == std::string::npos) return false;
    
    request.requestLine = parseRequestLine(data.substr(0, pos));
    
    size_t headerStart = pos + 2;
    size_t headerEnd = data.find("\r\n\r\n", headerStart);
    if (headerEnd == std::string::npos) return false;
    
    request.headers = parseHeaders(data.substr(headerStart, headerEnd - headerStart));
    
    size_t bodyStart = headerEnd + 4;
    if (bodyStart < data.length()) {
      request.body = data.substr(bodyStart);
    }
    
    auto connectionHeader = getHeader(request.headers, "Connection");
    request.keepAlive = (connectionHeader == "keep-alive" || 
                        (request.requestLine.version == "HTTP/1.1" && connectionHeader != "close"));
    
    return true;
  } catch (const std::exception &e) {
    return false;
  }
}

RequestLine parseRequestLine(const std::string &line) {
  RequestLine requestLine;
  
  const char* data = line.c_str();
  const char* end = data + line.length();
  const char* current = data;
  
  const char* methodEnd = StringUtils::findNextSpace(current, end);
  if (methodEnd < end) {
    std::string methodStr = StringUtils::extractToken(current, methodEnd);
    requestLine.method = stringToMethod(methodStr);
    
    current = StringUtils::skipWhitespace(methodEnd, end);
    const char* uriEnd = StringUtils::findNextSpace(current, end);
    if (uriEnd < end) {
      requestLine.uri = StringUtils::extractToken(current, uriEnd);
      current = StringUtils::skipWhitespace(uriEnd, end);
      requestLine.version = StringUtils::extractToken(current, end);
    }
  }
  
  return requestLine;
}

std::map<std::string, std::string> parseHeaders(const std::string &headerSection) {
  std::map<std::string, std::string> headers;
  
  const char* data = headerSection.c_str();
  const char* end = data + headerSection.length();
  const char* lineStart = data;
  
  while (lineStart < end) {
    const char* lineEnd = StringUtils::findLineEnd(lineStart, end);
    if (lineEnd == lineStart) break;
    
    StringUtils::parseHeaderLine(lineStart, lineEnd, headers);
    lineStart = StringUtils::skipLineTerminators(lineEnd, end);
  }
  
  return headers;
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
    }
  }
  
  response += "\r\n" + body;
  return response;
}

std::string createSimpleResponse(StatusCode status, const std::string &message) {
  std::map<std::string, std::string> headers;
  headers["Content-Type"] = "text/plain";
  headers["Content-Length"] = std::to_string(message.size());
  return buildResponse(status, headers, message);
}

std::string createErrorResponse(StatusCode status, const std::string &errorMessage) {
  std::string body = errorMessage;
  if (body.empty()) {
    std::string statusStr = std::to_string(static_cast<int>(status));
    std::string statusText = statusToString(status);
    body = "<html><body><h1>" + statusStr + " " + statusText + "</h1></body></html>";
  }
  
  std::map<std::string, std::string> headers;
  headers["Content-Type"] = "text/html";
  headers["Content-Length"] = std::to_string(body.size());
  return buildResponse(status, headers, body);
}

std::string createFileResponse(StatusCode status, const std::string &content, const std::string &contentType) {
  std::map<std::string, std::string> headers;
  headers["Content-Type"] = contentType;
  headers["Content-Length"] = std::to_string(content.size());
  
  // Simple cache control for successful responses
  if (status == StatusCode::OK) {
    headers["Cache-Control"] = "public, max-age=3600";
  }
  
  return buildResponse(status, headers, content);
}

}
