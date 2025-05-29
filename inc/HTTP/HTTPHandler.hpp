#pragma once
#include "resource/CGIHandler.hpp"
#include "HTTP/HTTPResponse.hpp"
#include "HTTP/HTTPTypes.hpp"
#include "HTTP/HTTPRequest.hpp"
#include "config/ServerConfig.hpp"
#include <map>
#include <memory>
#include <string>
class HTTPHandler {
private:
  std::string _root_directory;
  std::map<HTTP::StatusCode, std::string> _custom_error_pages;
  CGIHandler _cgiHandler;
  const ServerConfig* _config;
  std::string getGenericErrorMessage(HTTP::StatusCode status) const;
  std::unique_ptr<HTTPResponse> generateErrorResponse(HTTP::StatusCode status);
  std::unique_ptr<HTTPResponse> serveResource(const std::string &uri);
public:
  HTTPHandler(const std::string &root = "./www", const ServerConfig* config = nullptr);
  ~HTTPHandler();
  std::unique_ptr<HTTPResponse> handleRequest(const std::string &requestData);
  std::unique_ptr<HTTPResponse> handleGET(const HTTPRequest &request);
  std::unique_ptr<HTTPResponse> handlePOST(const HTTPRequest &request);
  std::unique_ptr<HTTPResponse> handleDELETE(const HTTPRequest &request);
  std::unique_ptr<HTTPResponse> handleError(HTTP::StatusCode status);
  void setRootDirectory(const std::string &root);
};
