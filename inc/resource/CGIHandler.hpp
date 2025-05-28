#pragma once
#include "HTTPResponse.hpp"
#include "HTTPRequest.hpp"
#include <map>
#include <memory>
#include <string>
class CGIHandler {
private:
  std::string _root_directory;
  std::map<std::string, std::string> _cgi_handlers;
  std::unique_ptr<HTTPResponse> executeScript(const std::string& script_path, const std::string& handler_path, const HTTPRequest& request);

public:
  CGIHandler(const std::string &root = "./www");
  ~CGIHandler();
  std::unique_ptr<HTTPResponse> executeCGI(const std::string &uri,
                                           const HTTPRequest &request);
  void registerHandler(const std::string &extension,
                       const std::string &handlerPath);
  bool canHandle(const std::string &filePath) const;
  void setRootDirectory(const std::string &root);
};
