#include "IHTTPRequest.hpp"
#include "HTTPGetRequest.hpp"
#include <iostream>

HTTPParseException::HTTPParseException(const std::string &message)
    : std::runtime_error(message) {}

HTTP::RequestLine IHTTPRequest::parseRequestLine(const std::string &line) {
  HTTP::RequestLine res;

  size_t first_space = line.find(' ');
  if (first_space == std::string::npos)
    throw HTTPParseException("Invalid request line: missing URI and version");

  std::string method_str = line.substr(0, first_space);
  try {
    res.method = HTTP::stringToMethod(method_str);
  } catch (const std::exception &e) throw HTTPParseException(
      "Invalid method: " + method_str);

  size_t last_space = line.find(' ', first_space + 1);
  if (last_space == std::string::npos)
    throw HTTPParseException("Invalid request line: missing version");

  res.uri = line.substr(first_space + 1, last_space - first_space - 1);
  res.version = line.substr(last_space + 1);
  if (!HTTP::isValidMethod(HTTP::methodToString(res.method)) ||
      !HTTP::isValidUri(res.uri) || !HTTP::isValidVersion(res.version)) {
    throw HTTPParseException("Invalid request line components");
  }
  return res;
}

std::map<std::string, std::string>
IHTTPRequest::parseHeaders(const std::string &headerSection) {
  std::map<std::string, std::string> headers;
  size_t pos = 0;
  size_t prev = 0;

  while ((pos = headerSection.find("\r\n", prev)) != std::string::npos) {
    std::string line = headerSection.substr(prev, pos - prev);
    if (line.empty())
      break;
    size_t colonPos = line.find(':');
    if (colonPos != std::string::npos) {
      std::string key = line.substr(0, colonPos);
      std::string value = line.substr(colonPos + 1);
      size_t valueStart = value.find_first_not_of(" \t");
      if (valueStart != std::string::npos)
        value = value.substr(valueStart);
      headers[key] = value;
    } else
      throw HTTPParseException("Invalid header format: " + line);
    prev = pos + 2;
  }
  return headers;
}

std::unique_ptr<IHTTPRequest>
IHTTPRequest::createRequest(const std::string &requestData) {
  try {
    size_t endOfLine = requestData.find("\r\n");
    if (endOfLine == std::string::npos) {
      throw HTTPParseException("Invalid request format: no CRLF found");
    }

    std::string requestLine = requestData.substr(0, endOfLine);
    size_t firstSpace = requestLine.find(' ');
    if (firstSpace == std::string::npos) {
      throw HTTPParseException("Invalid request line: no method found");
    }

    std::string methodStr = requestLine.substr(0, firstSpace);
    if (methodStr == "GET") {
      auto request = std::make_unique<HTTPGetRequest>();
      if (request->parseRequest(requestData))
        return request;
    } else
      throw HTTPParseException("Unsupported HTTP method: " + methodStr);
  } catch (const HTTPParseException &e) std::cerr
      << "Error creating request: " << e.what() << std::endl;
  catch (const std::exception &e) std::cerr
      << "Unexpected error creating request: " << e.what() << std::endl;
  return nullptr;
}
