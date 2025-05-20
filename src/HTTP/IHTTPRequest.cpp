#include "IHTTPRequest.hpp"

HTTPParseException::HTTPParseException(const std::string &message)
    : std::runtime_error(message) {}

IHTTPRequest::Method IHTTPRequest::stringToMethod(const std::string &method) {
  if (method == "GET") {
    return Method::GET;
  } else if (method == "POST") {
    return Method::POST;
  } else if (method == "DELETE") {
    return Method::DELETE;
  } else {
    throw HTTPParseException("Invalid HTTP method: " + method);
  }
}

std::string IHTTPRequest::methodToString(Method method) {
  switch (method) {
  case Method::GET:
    return "GET";
  case Method::POST:
    return "POST";
  case Method::DELETE:
    return "DELETE";
  default:
    throw HTTPParseException("Invalid HTTP method");
  }
}

IHTTPRequest::RequestLine
IHTTPRequest::parseRequestLine(const std::string &line) {
  size_t first_space = line.find(' ');
  size_t last_space = line.rfind(' ');

  if (first_space == std::string::npos || last_space == std::string::npos ||
      first_space == last_space) {
    throw HTTPParseException("Invalid request line: " + line);
  }

  RequestLine res;
  res.method = stringToMethod(line.substr(0, first_space));
  res.uri = line.substr(first_space + 1, last_space - first_space - 1);
  res.version = line.substr(last_space + 1);

  if (!isValidMethod(methodToString(res.method)) || !isValidUri(res.uri) ||
      !isValidVersion(res.version)) {
    throw HTTPParseException("Invalid request line: " + line);
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
      std::string value = line.substr(colonPos + 2);
      headers[key] = value;
    } else {
      throw HTTPParseException("Invalid header line: " + line);
    }
    prev = pos + 2; // Skip "\r\n"
  }

  return headers;
}

bool IHTTPRequest::isValidVersion(const std::string &version) {
  return version == "HTTP/1.0" || version == "HTTP/1.1";
}

bool IHTTPRequest::isValidMethod(const std::string &method) {
  return method == "GET" || method == "POST"||
         method == "DELETE";
}

bool IHTTPRequest::isValidUri(const std::string &uri) {
  return !uri.empty() && uri[0] == '/';
}

std::unique_ptr<IHTTPRequest>
IHTTPRequest::createRequest(const std::string &requestData) {
	(void) requestData;
  // Parse the request data and create an appropriate IHTTPRequest object
  // This is a placeholder implementation. You should implement the actual
  // parsing logic here.
  return nullptr;
}

IHTTPRequest::RequestLine &IHTTPRequest::RequestLine::operator=(const RequestLine &other) {
  if (this != &other) {
    method = other.method;
    uri = other.uri;
    version = other.version;
  }
  return *this;
}
