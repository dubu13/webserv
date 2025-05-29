#pragma once
#include "HTTP/HTTP.hpp"
#include "config/ServerConfig.hpp"
#include "resource/CGIHandler.hpp"
#include "resource/FileHandler.hpp"
#include <map>
#include <string>
class HTTPHandler {
private:
  std::string _root_directory;
  std::map<HTTP::StatusCode, std::string> _custom_error_pages;
  CGIHandler _cgiHandler;
  const ServerConfig *_config;

public:
  HTTPHandler(const std::string &root = "./www",
              const ServerConfig *config = nullptr);
  ~HTTPHandler();
  std::string handleRequest(const std::string &requestData);
  void setRootDirectory(const std::string &root);
};
