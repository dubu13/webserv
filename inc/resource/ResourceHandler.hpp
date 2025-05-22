#pragma once

#include "ErrorHandler.hpp"
#include "FileType.hpp"
#include "HTTPResponse.hpp"
#include <memory>
#include <string>

class ResourceHandler {
private:
  std::string _root_directory;
  ErrorHandler *_errorHandler;

public:
  ResourceHandler(const std::string &root = "./www",
                  ErrorHandler *errorHandler = nullptr);
  ~ResourceHandler();

  std::unique_ptr<HTTPResponse> serveResource(const std::string &uri);

  void setRootDirectory(const std::string &root);
  void setErrorHandler(ErrorHandler *errorHandler);

  std::string getRootDirectory() const;
};
