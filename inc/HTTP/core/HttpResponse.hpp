#pragma once
#include "utils/Utils.hpp"
#include <filesystem>
#include <map>
#include <string>
#include <string_view>

class HttpResponse {
public:
  HttpResponse() = default;

  int statusCode() const { return _statusCode; }
  const std::string &statusText() const { return _statusText; }
  const std::string &body() const { return _body; }
  const std::map<std::string, std::string> &headers() const { return _headers; }

  std::string str() const;

  operator std::string() const { return str(); }

  HttpResponse &status(int code, std::string_view text = "");
  HttpResponse &header(std::string_view name, std::string_view value);
  HttpResponse &body(std::string_view content,
                     std::string_view contentType = "text/plain");

  template <typename... Headers> HttpResponse &headers(Headers &&...headers) {
    (setHeader(std::forward<Headers>(headers)), ...);
    return *this;
  }

  static std::string ok(std::string_view content = "",
                        std::string_view contentType = "text/plain");
  static HttpResponse redirect(std::string_view location, int code = 302);

  static HttpResponse file(std::string_view filePath);
  static HttpResponse directory(std::string_view dirPath, std::string_view uri);

  static std::string buildResponse(int statusCode,
                                   const std::string &statusText,
                                   std::string_view content,
                                   std::string_view contentType);

private:
  static std::string formatDate();
  void setHeader(const std::pair<std::string_view, std::string_view> &header);

  int _statusCode{200};
  std::string _statusText{"OK"};
  std::string _body;
  std::map<std::string, std::string> _headers;
};
