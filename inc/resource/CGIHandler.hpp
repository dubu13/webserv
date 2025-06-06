#pragma once
#include "HTTP/core/HTTPParser.hpp"
#include "HTTP/core/HTTPTypes.hpp"
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using HTTP::Request;

class CGIHandler {
private:
  std::string _root_directory;
  std::map<std::string, std::string> _cgi_handlers;
  std::string executeScript(const std::string &script_path,
                            const std::string &handler_path,
                            const Request &request);

public:
  CGIHandler(const std::string &root = "./www");
  ~CGIHandler();
  std::string executeCGI(const std::string &uri, const Request &request);
  void registerHandler(const std::string &extension,
                       const std::string &handlerPath);
  bool canHandle(const std::string &filePath) const;
  void setRootDirectory(const std::string &root);
  std::string parseCGIOutput(const std::string &output);
};
