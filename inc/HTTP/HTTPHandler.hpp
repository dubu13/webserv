#pragma once
#include "HTTP/HTTPTypes.hpp"
#include "HTTP/HttpParser.hpp"
#include "config/Config.hpp"
#include "resource/CGIHandler.hpp"
#include "utils/FileUtils.hpp"
#include <map>
#include <string>
class HTTPHandler {
private:
  std::string _root_directory;
  std::map<HTTP::StatusCode, std::string> _custom_error_pages;
  CGIHandler _cgiHandler;
  const ServerBlock *_config;
  
  // Helper methods
  std::string handleCgiRequest(const HttpParser::Request& request, const std::string& effectiveRoot, const std::string& effectiveUri);
  std::string createErrorResponse(HTTP::StatusCode status, const std::string& message = "");
  
public:
  HTTPHandler(const std::string &root = "./www",
              const ServerBlock *config = nullptr);
  ~HTTPHandler();
  std::string handleRequest(const std::string &requestData);
  void setRootDirectory(const std::string &root);
};
