#pragma once
#include "HTTPTypes.hpp"
#include <map>
#include <string>

namespace HTTP {

struct Request {
  RequestLine requestLine;
  std::map<std::string, std::string> headers;
  std::string body;
  bool keepAlive = false;
  Request() = default;
};

bool parseRequest(const std::string &data, Request &request);
RequestLine parseRequestLine(const std::string &line);
std::map<std::string, std::string> parseHeaders(const std::string &headerSection);

std::string getHeader(const std::map<std::string, std::string> &headers, const std::string &key);
void setHeader(std::map<std::string, std::string> &headers, const std::string &key, const std::string &value);
bool hasHeader(const std::map<std::string, std::string> &headers, const std::string &key);

std::string buildResponse(StatusCode status, const std::map<std::string, std::string> &headers, 
                         const std::string &body, bool keepAlive = false);
std::string createSimpleResponse(StatusCode status, const std::string &message);
std::string createErrorResponse(StatusCode status, const std::string &errorMessage = "");
std::string createFileResponse(StatusCode status, const std::string &content, const std::string &contentType);

}
