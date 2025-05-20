#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <memory>

class HTTPParseException : public std::runtime_error {
public:
  explicit HTTPParseException(const std::string &message);
};

class IHTTPRequest {
public:
  enum class Method { GET, POST, DELETE };
  struct RequestLine {
    Method method;
    std::string uri;
    std::string version;
		RequestLine &operator=(const RequestLine &other);
  };

  virtual ~IHTTPRequest() = default;

  // pure virtual methods
  virtual bool parseRequest(const std::string &data) = 0;
  virtual Method getMethod() const = 0;
  virtual std::string getUri() const = 0;
  virtual std::string getVersion() const = 0;
  virtual std::string getBody() const = 0;
  virtual std::string getHeader(const std::string &key) const = 0;
  virtual std::map<std::string, std::string> getHeaders() const = 0;

  // Factory method

  static std::unique_ptr<IHTTPRequest>
  createRequest(const std::string &requestData);

protected:
  static Method stringToMethod(const std::string &method);
  static std::string methodToString(Method method);
  static RequestLine parseRequestLine(const std::string &line);
  static std::map<std::string, std::string>
  parseHeaders(const std::string &headers);
  static std::string parseBody(const std::string &body);
  static bool isValidMethod(const std::string &method);
  static bool isValidUri(const std::string &uri);
  static bool isValidVersion(const std::string &version);
};
