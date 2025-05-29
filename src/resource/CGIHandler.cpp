#include "CGIHandler.hpp"
#include "HTTP/HTTP.hpp"
#include <iostream>
CGIHandler::CGIHandler(const std::string &root) : _root_directory(root) {
  registerHandler(".php", "/usr/bin/php");
  registerHandler(".py", "/usr/bin/python");
  registerHandler(".pl", "/usr/bin/perl");
}
CGIHandler::~CGIHandler() {}
std::string CGIHandler::executeCGI(const std::string &uri,
                                   const HTTP::Request &request) {
  std::string filePath = _root_directory + uri;
  size_t dot_pos = filePath.find_last_of('.');
  if (dot_pos == std::string::npos) {
    return "";
  }
  std::string extension = filePath.substr(dot_pos);
  auto handlerIt = _cgi_handlers.find(extension);
  if (handlerIt == _cgi_handlers.end()) {
    return "";
  }
  return executeScript(filePath, handlerIt->second, request);
}
void CGIHandler::registerHandler(const std::string &extension,
                                 const std::string &handlerPath) {
  _cgi_handlers[extension] = handlerPath;
}
bool CGIHandler::canHandle(const std::string &filePath) const {
  size_t dot_pos = filePath.find_last_of('.');
  if (dot_pos == std::string::npos) {
    return false;
  }
  std::string extension = filePath.substr(dot_pos);
  return _cgi_handlers.find(extension) != _cgi_handlers.end();
}
void CGIHandler::setRootDirectory(const std::string &root) {
  _root_directory = root;
}
std::string CGIHandler::executeScript(const std::string &script_path,
                                      const std::string &handler_path,
                                      const HTTP::Request &request) {
  (void)script_path;
  (void)handler_path;
  (void)request;
  return HTTP::createSimpleResponse(HTTP::StatusCode::OK,
                                    "CGI execution not implemented yet");
}
