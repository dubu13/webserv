#include "HTTP/HTTP.hpp"
#include "resource/FileHandler.hpp"
#include <iostream>
#include <sstream>
namespace HTTP {
bool parseRequest(const std::string &data, Request &request) {
  try {
    size_t pos = data.find("\r\n");
    if (pos == std::string::npos) {
      return false;
    }
    request.requestLine = parseRequestLine(data.substr(0, pos));
    size_t headerStart = pos + 2;
    size_t headerEnd = data.find("\r\n\r\n", headerStart);
    if (headerEnd == std::string::npos) {
      return false;
    }
    request.headers =
        parseHeaders(data.substr(headerStart, headerEnd - headerStart));
    size_t bodyStart = headerEnd + 4;
    if (bodyStart < data.length()) {
      request.body = data.substr(bodyStart);
    }
    return true;
  } catch (const std::exception &e) {
    return false;
  }
}
RequestLine parseRequestLine(const std::string &line) {
  RequestLine requestLine;
  std::string::size_type start = 0;
  std::string::size_type end = line.find(' ');
  if (end != std::string::npos) {
    std::string methodStr = line.substr(start, end - start);
    requestLine.method = stringToMethod(methodStr);
    start = end + 1;
    end = line.find(' ', start);
    if (end != std::string::npos) {
      requestLine.uri = line.substr(start, end - start);
      start = end + 1;
      requestLine.version = line.substr(start);
    }
  }
  return requestLine;
}
std::map<std::string, std::string>
parseHeaders(const std::string &headerSection) {
  std::map<std::string, std::string> headers;
  std::istringstream stream(headerSection);
  std::string line;
  while (std::getline(stream, line) && !line.empty()) {
    if (line.back() == '\r')
      line.pop_back();
    size_t colonPos = line.find(':');
    if (colonPos != std::string::npos) {
      std::string key = line.substr(0, colonPos);
      std::string value = line.substr(colonPos + 1);
      if (!value.empty() && value[0] == ' ')
        value = value.substr(1);
      headers[key] = value;
    }
  }
  return headers;
}
std::string getHeader(const std::map<std::string, std::string> &headers,
                      const std::string &key) {
  auto it = headers.find(key);
  return (it != headers.end()) ? it->second : "";
}
void setHeader(std::map<std::string, std::string> &headers,
               const std::string &key, const std::string &value) {
  headers[key] = value;
}
bool hasHeader(const std::map<std::string, std::string> &headers,
               const std::string &key) {
  return headers.find(key) != headers.end();
}
void removeHeader(std::map<std::string, std::string> &headers,
                  const std::string &key) {
  headers.erase(key);
}
void clearHeaders(std::map<std::string, std::string> &headers) {
  headers.clear();
}
void updateContentLength(std::map<std::string, std::string> &headers,
                         size_t bodySize) {
  setHeader(headers, "Content-Length", std::to_string(bodySize));
}
std::string generateResponse(StatusCode status,
                             const std::map<std::string, std::string> &headers,
                             const std::string &body,
                             const std::string &version) {
  std::stringstream response;
  response << version << " " << static_cast<int>(status) << " "
           << statusToString(status) << "\r\n";
  for (const auto &header : headers) {
    response << header.first << ": " << header.second << "\r\n";
  }
  response << "\r\n" << body;
  return response.str();
}
std::string createSimpleResponse(StatusCode status,
                                 const std::string &message) {
  auto headers = createStandardHeaders("text/plain");
  updateContentLength(headers, message.size());
  return generateResponse(status, headers, message);
}
std::string createErrorResponse(StatusCode status,
                                const std::string &errorMessage) {
  std::string body = errorMessage;
  if (body.empty()) {
    std::string statusStr = std::to_string(static_cast<int>(status));
    std::string statusText = statusToString(status);
    body = "<html><body><h1>" + statusStr + " " + statusText +
           "</h1></body></html>";
  }
  auto headers = createStandardHeaders("text/html");
  updateContentLength(headers, body.size());
  return generateResponse(status, headers, body);
}
std::string createFileResponse(StatusCode status, const std::string &content,
                               const std::string &contentType) {
  auto headers = createStandardHeaders(contentType);
  updateContentLength(headers, content.size());
  return generateResponse(status, headers, content);
}
std::map<std::string, std::string>
createStandardHeaders(const std::string &contentType) {
  std::map<std::string, std::string> headers;
  setHeader(headers, "Connection", "close");
  setHeader(headers, "Content-Type", contentType);
  return headers;
}
std::map<std::string, std::string>
createFileHeaders(const std::string &filePath) {
  return createStandardHeaders(FileOps::getMimeType(filePath));
}
} // namespace HTTP
