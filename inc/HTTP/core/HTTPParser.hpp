#pragma once

#include "HTTPTypes.hpp"
#include "utils/Logger.hpp"
#include "utils/Utils.hpp"
#include <map>
#include <string>
#include <string_view>

// Forward declarations
class RequestRouter;

namespace HTTP {

struct RequestLine {
  Method method;
  std::string uri;
  std::string version;
};

struct Request {
  RequestLine requestLine;
  std::map<std::string, std::string> headers;
  std::string body;
  bool keepAlive = false;
  bool isChunked = false;

  size_t contentLength = 0;
  bool chunkedTransfer = false;
  std::string contentType;
};

struct ParseResult {
  bool success;
  int statusCode;  // HTTP status code for error responses
  std::string errorMessage;
  
  ParseResult(bool success = true, int statusCode = 200, const std::string& errorMessage = "") 
    : success(success), statusCode(statusCode), errorMessage(errorMessage) {}
};

ParseResult parseRequest(const std::string &data, Request &request, const RequestRouter *router = nullptr);
bool parseRequestLine(std::string_view line, RequestLine &requestLine);
bool parseHeaders(std::istringstream &stream,
                  std::map<std::string, std::string> &headers);
bool parseHeader(std::string_view line,
                 std::map<std::string, std::string> &headers);
bool parseBody(std::string_view data, size_t bodyStart, std::string &body);
bool parseChunkedBody(std::string_view data, size_t bodyStart,
                     std::string &body, const std::string &uri, const RequestRouter *router);
bool parseRequestBody(const std::string &data, size_t bodyStart,
                     Request &request, const RequestRouter *router);
bool parseContentLengthWithRouter(Request &request, const RequestRouter *router);
bool validateHttpRequest(const Request &request);

struct MultipartFile {
  std::string filename;
  std::string content;
};

std::vector<MultipartFile> parseMultipartData(const std::string &body,
                                              std::string_view contentType);

} // namespace HTTP
