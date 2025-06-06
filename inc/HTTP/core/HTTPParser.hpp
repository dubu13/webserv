#pragma once

#include "HTTPTypes.hpp"
#include "utils/Logger.hpp"
#include "utils/Utils.hpp"
#include <map>
#include <string>
#include <string_view>

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

bool parseRequest(const std::string &data, Request &request);
bool parseRequestLine(std::string_view line, RequestLine &requestLine);
bool parseHeaders(std::istringstream &stream,
                  std::map<std::string, std::string> &headers);
bool parseHeader(std::string_view line,
                 std::map<std::string, std::string> &headers);
bool parseBody(std::string_view data, size_t bodyStart, std::string &body);
bool parseChunkedBody(std::string_view data, size_t bodyStart,
                      std::string &body);
bool parseRequestBody(const std::string &data, size_t bodyStart,
                      Request &request);
bool parseContentLength(Request &request);
bool validateHttpRequest(const Request &request);

struct MultipartFile {
  std::string filename;
  std::string content;
};

std::vector<MultipartFile> parseMultipartData(const std::string &body,
                                              std::string_view contentType);

} // namespace HTTP
