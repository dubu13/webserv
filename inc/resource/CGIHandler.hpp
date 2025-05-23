#pragma once

#include "CGIExecutor.hpp"
#include "ErrorHandler.hpp"
#include "FileType.hpp"
#include "HTTPResponse.hpp"
#include "IHTTPRequest.hpp"
#include <map>
#include <memory>
#include <string>

class CGIHandler {
private:
  std::string _root_directory;
  std::map<std::string, std::string> _cgi_handlers;
  ErrorHandler *_errorHandler;
  CGIExecutor _executor;

public:
  CGIHandler(const std::string &root = "./www",
             ErrorHandler *errorHandler = nullptr);
  ~CGIHandler();

  std::unique_ptr<HTTPResponse> executeCGI(const std::string &uri,
                                           const IHTTPRequest &request);

  void registerHandler(const std::string &extension,
                       const std::string &handlerPath);
  void unregisterHandler(const std::string &extension);

  bool canHandle(const std::string &filePath) const;

  void setRootDirectory(const std::string &root);
  void setErrorHandler(ErrorHandler *errorHandler);
};
