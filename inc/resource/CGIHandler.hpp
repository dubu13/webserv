#pragma once
#include "HTTP/HTTPTypes.hpp"
#include "HTTP/HttpParser.hpp"
#include <map>
#include <string>
class CGIHandler {
private:
  std::string _root_directory;
  std::map<std::string, std::string> _cgi_handlers;
  std::string executeScript(const std::string &script_path,
                            const std::string &handler_path,
                            const HttpParser::Request &request);
public:
  CGIHandler(const std::string &root = "./www");
  ~CGIHandler();
  std::string executeCGI(const std::string &uri, const HttpParser::Request &request);
  void registerHandler(const std::string &extension,
                       const std::string &handlerPath);
  bool canHandle(const std::string &filePath) const;
  void setRootDirectory(const std::string &root);
};
