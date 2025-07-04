#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>

#include "HTTP/core/HTTPParser.hpp"
#include "HTTP/core/HTTPTypes.hpp"
#include "HTTP/routing/RequestRouter.hpp"
#include "Logger.hpp"
#include "utils/Utils.hpp"
#include "utils/ValidationUtils.hpp"

namespace HTTP {

static void cleanLineEnding(std::string &line) {
  if (!line.empty() && line.back() == '\r')
    line.pop_back();
}

static std::string getHeader(const std::map<std::string, std::string> &headers,
                             const std::string &name) {
  auto it = headers.find(name);
  return it != headers.end() ? it->second : "";
}

static bool isValidHttpVersion(const std::string &version) {
  return version == "HTTP/1.1" || version == "HTTP/1.0";
}

ParseResult parseRequest(const std::string &data, Request &request, const RequestRouter *router) {
  if (data.empty()) {
    Logger::error("Empty HTTP request");
    return ParseResult(false, 400, "Bad Request");
  }
  size_t headerEnd = data.find("\r\n\r\n");
  if (headerEnd == std::string::npos) {
    if (data.length() > 8192) {
      Logger::error("Malformed HTTP request: headers too large or missing separator");
      return ParseResult(false, 400, "Bad Request");
    }
    Logger::error("Malformed HTTP request: missing header-body separator");
    return ParseResult(false, 400, "Bad Request");
  }

  if (headerEnd > 8192) {
    Logger::error("HTTP request headers too large");
    return ParseResult(false, 431, "Request Header Fields Too Large");
  }

  std::istringstream stream(data.substr(0, headerEnd));
  std::string requestLineStr;

  if (!std::getline(stream, requestLineStr)) {
    Logger::error("Failed to read request line");
    return ParseResult(false, 400, "Bad Request");
  }
  
  cleanLineEnding(requestLineStr);
  if (requestLineStr.empty()) {
    Logger::error("Empty request line");
    return ParseResult(false, 400, "Bad Request");
  }

  if (!parseRequestLine(requestLineStr, request.requestLine))
    return ParseResult(false, 400, "Bad Request");
  
  if (!parseHeaders(stream, request.headers))
    return ParseResult(false, 400, "Bad Request");
  
  if (!validateHttpRequest(request))
    return ParseResult(false, 400, "Bad Request");
  
  if (router) {
    const LocationBlock *location = router->findLocation(request.requestLine.uri);
    if (location && !router->isMethodAllowed(request, location)) {
      return ParseResult(false, 405, "Method Not Allowed");
    }
  }
  
  if (!parseContentLengthWithRouter(request, router))
    return ParseResult(false, 413, "Payload Too Large");

  if (!parseRequestBody(data, headerEnd + 4, request, router)) {
    size_t maxBodySize = ValidationUtils::getMaxBodySize(request.requestLine.uri, router);
    if (request.body.length() > maxBodySize)
      return ParseResult(false, 413, "Payload Too Large");
    return ParseResult(false, 400, "Bad Request");
  }
  
  return ParseResult(true, 200, "OK");
}

bool parseRequestLine(std::string_view line, RequestLine &requestLine) {
  if (line.empty()) {
    Logger::error("Empty request line");
    return false;
  }

  std::istringstream stream{std::string{line}};
  std::string methodStr;

  if (!(stream >> methodStr >> requestLine.uri >> requestLine.version)) {
    Logger::error("Invalid request line format - missing method, URI, or version");
    return false;
  }

  std::string extra;
  if (stream >> extra) {
    Logger::error("Invalid request line format - too many parts");
    return false;
  }

  requestLine.method = stringToMethod(methodStr);
  if (requestLine.method == Method::UNKNOWN) {
    Logger::error("Invalid HTTP method: " + methodStr);
    return false;
  }

  if (requestLine.uri.empty() || requestLine.uri[0] != '/') {
    Logger::error("Invalid URI: must start with /");
    return false;
  }
  if (requestLine.uri.length() > Constants::MAX_URI_LENGTH) {
    Logger::error("URI too long");
    return false;
  }
  if (!ValidationUtils::isPathSafe(requestLine.uri)) {
    Logger::error("Unsafe URI path");
    return false;
  }

  if (!isValidHttpVersion(requestLine.version)) {
    Logger::error("Invalid HTTP version: " + requestLine.version);
    return false;
  }

  return true;
}

bool parseHeaders(std::istringstream &stream,
                  std::map<std::string, std::string> &headers) {
  std::string line;
  while (std::getline(stream, line) && !line.empty()) {
    cleanLineEnding(line);
    if (!parseHeader(line, headers))
      return false;
  }
  return true;
}

bool parseHeader(std::string_view line,
                 std::map<std::string, std::string> &headers) {
  size_t colonPos = line.find(':');
  if (colonPos == std::string::npos) {
    Logger::error("Invalid header format");
    return false;
  }

  headers[std::string(HttpUtils::trimWhitespace(line.substr(0, colonPos)))] =
      std::string(HttpUtils::trimWhitespace(line.substr(colonPos + 1)));
  return true;
}

bool parseContentLengthWithRouter(Request &request, const RequestRouter *router) {
  if (router) {
    const LocationBlock *location = router->findLocation(request.requestLine.uri);
    if (location && !router->isMethodAllowed(request, location)) {
      return true;
    }
  }
  
  std::string contentLength = getHeader(request.headers, "Content-Length");
  if (contentLength.empty())
    return true;
  
  if (!ValidationUtils::validateContentLengthWithRouter(contentLength, request.requestLine.uri, request.contentLength, router)) {
    Logger::error("Invalid Content-Length or body size exceeds limit");
    return false;
  }
  return true;
}

bool validateHttpRequest(const Request &request) {
  if (!isValidHttpVersion(request.requestLine.version)) {
    Logger::error("Unsupported version " + request.requestLine.version);
    return false;
  }
  if (request.requestLine.version == "HTTP/1.1" && getHeader(request.headers, "Host").empty()) {
    Logger::error("Missing Host header for HTTP/1.1");
    return false;
  }
  return true;
}

bool parseRequestBody(const std::string &data, size_t bodyStart,
                     Request &request, const RequestRouter *router) {
  if (bodyStart >= data.length()) {
    request.body.clear();
    return true;
  }
  size_t maxBodySize = ValidationUtils::getMaxBodySize(request.requestLine.uri, router);

  std::string transferEncoding = getHeader(request.headers, "Transfer-Encoding");
  if (!transferEncoding.empty() && transferEncoding == "chunked") {
    request.chunkedTransfer = true;
    return parseChunkedBody(data, bodyStart, request.body, request.requestLine.uri, router);
  }

  if (!parseBody(data, bodyStart, request.body)) {
    return false;
  }

  if (request.body.length() > maxBodySize) {
    Logger::error("HTTP/1.1 Error: Body size exceeds configured limit");
    return false;
  }

  return true;
}

bool parseChunkedBody(std::string_view data, size_t bodyStart,
                     std::string &body, const std::string &uri, const RequestRouter *router) {
  body.clear();
  size_t pos = bodyStart, chunkCount = 0;
  size_t maxBodySize = ValidationUtils::getMaxBodySize(uri, router);

  while (pos < data.length()) {
    size_t chunkSize;

    if (++chunkCount > Constants::MAX_CHUNK_COUNT) {
      Logger::error("HTTP/1.1 Error: Too many chunks");
      return false;
    }
    if (!HttpUtils::parseChunkSize(data, pos, chunkSize) ||
        pos + chunkSize + 2 > data.length()) {
      Logger::error("HTTP/1.1 Error: Incomplete chunk data");
      return false;
    }
    if (chunkSize == 0)
      return true;
    body.append(data.substr(pos, chunkSize));
    pos += chunkSize;
    if (!ValidationUtils::validateChunkTerminator(data, pos))
      return false;
    if (body.length() > maxBodySize) {
      Logger::error("HTTP/1.1 Error: Body size exceeds configured limit");
      return false;
    }
    pos += 2;
  }
  Logger::error("HTTP/1.1 Error: Malformed chunked body");
  return false;
}

bool parseBody(std::string_view data, size_t bodyStart, std::string &body) {
  body = bodyStart < data.length() ? std::string(data.substr(bodyStart)) : "";
  return true;
}

static std::string extractBoundary(std::string_view contentType) {
  size_t boundaryPos = contentType.find("boundary=");
  if (boundaryPos == std::string_view::npos)
    return "";
  boundaryPos += 9;
  size_t boundaryEnd = contentType.find(";", boundaryPos);
  if (boundaryEnd == std::string_view::npos)
    boundaryEnd = contentType.length();
  std::string boundary =
      std::string(contentType.substr(boundaryPos, boundaryEnd - boundaryPos));
  if (boundary.length() >= 2 && boundary.front() == '"' &&
      boundary.back() == '"')
    boundary = boundary.substr(1, boundary.length() - 2);
  return boundary;
}

std::vector<MultipartFile> parseMultipartData(const std::string &body,
                                              std::string_view contentType) {
  std::vector<MultipartFile> files;
  std::string boundary = extractBoundary(contentType);

  if (boundary.empty()) {
    Logger::error("No boundary found in multipart content-type");
    return files;
  }

  std::string boundaryMarker = "--" + boundary;
  size_t pos = body.find(boundaryMarker);

  while (pos != std::string::npos) {
    pos += boundaryMarker.length();
    if (pos + 1 < body.length() && body.substr(pos, 2) == "--")
      break;
    pos = body.find("\r\n\r\n", pos);
    if (pos == std::string::npos)
      break;
    pos += 4;
    size_t nextBoundary = body.find("\r\n" + boundaryMarker, pos);
    if (nextBoundary == std::string::npos)
      break;
    MultipartFile file;
    size_t headerStart = body.rfind("filename=\"", pos);
    if (headerStart != std::string::npos && headerStart > pos - 200) {
      headerStart += 10;
      size_t headerEnd = body.find("\"", headerStart);
      if (headerEnd != std::string::npos)
        file.filename = body.substr(headerStart, headerEnd - headerStart);
    }
    file.content = body.substr(pos, nextBoundary - pos);
    if (!file.content.empty())
      files.push_back(std::move(file));
    pos = nextBoundary;
  }
  return files;
}
}
