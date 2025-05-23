#pragma once

#include "resource/CGIHandler.hpp"
#include "errors/ErrorHandler.hpp"
#include "HTTP/HTTPResponse.hpp"
#include "HTTP/HTTPTypes.hpp"
#include "HTTP/IHTTPRequest.hpp"
#include "resource/ResourceHandler.hpp"
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

  std::string sanitizeFilename(const std::string &filename);
  std::string extractFilenameFromRequest(const IHTTPRequest &request);

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
