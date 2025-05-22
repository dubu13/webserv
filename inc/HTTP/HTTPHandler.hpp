#pragma once

#include "CGIHandler.hpp"
#include "ErrorHandler.hpp"
#include "HTTPResponse.hpp"
#include "HTTPTypes.hpp"
#include "IHTTPRequest.hpp"
#include "ResourceHandler.hpp"
#include <ctime>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <string>

class HTTPHandler {
private:
  typedef std::unique_ptr<HTTPResponse> (HTTPHandler::*RequestHandlerFunction)(
      const IHTTPRequest &);

  std::string _root_directory;
  std::map<HTTP::Method, RequestHandlerFunction> _handlers;

  ErrorHandler _errorHandler;
  ResourceHandler _resourceHandler;
  CGIHandler _cgiHandler;


public:
  HTTPHandler(const std::string &root = "./www");
  ~HTTPHandler();

  std::unique_ptr<HTTPResponse> handleRequest(const std::string &requestData);
  std::unique_ptr<HTTPResponse> handleGET(const IHTTPRequest &request);
  std::unique_ptr<HTTPResponse> handlePOST(const IHTTPRequest &request);
  std::unique_ptr<HTTPResponse> handleDELETE(const IHTTPRequest &request);

  std::unique_ptr<HTTPResponse> handleError(HTTP::StatusCode status);

  void setRootDirectory(const std::string &root);

  ResourceHandler &getResourceHandler() { return _resourceHandler; }
  ErrorHandler &getErrorHandler() { return _errorHandler; }
};
