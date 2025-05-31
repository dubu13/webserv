#include "CGIHandler.hpp"
#include "HTTP/HTTPTypes.hpp"
#include "HTTP/HttpResponseBuilder.hpp"
#include "utils/Logger.hpp"
#include <iostream>

CGIHandler::CGIHandler(const std::string &root) : _root_directory(root) {
  Logger::infof("CGIHandler initialized with root directory: %s", root.c_str());
  registerHandler(".php", "/usr/bin/php");
  registerHandler(".py", "/usr/bin/python");
  registerHandler(".pl", "/usr/bin/perl");
  Logger::debugf("Registered %zu CGI handlers", _cgi_handlers.size());
}
CGIHandler::~CGIHandler() {}
std::string CGIHandler::executeCGI(const std::string &uri,
                                   const HttpParser::Request &request) {
  std::string filePath = _root_directory + uri;
  Logger::debugf("CGI execution requested for: %s", filePath.c_str());
  
  size_t dot_pos = filePath.find_last_of('.');
  if (dot_pos == std::string::npos) {
    Logger::debug("No file extension found - not a CGI script");
    return "";
  }
  
  std::string extension = filePath.substr(dot_pos);
  Logger::debugf("Script extension: %s", extension.c_str());
  
  auto handlerIt = _cgi_handlers.find(extension);
  if (handlerIt == _cgi_handlers.end()) {
    Logger::debugf("No CGI handler found for extension: %s", extension.c_str());
    return "";
  }
  
  Logger::infof("Executing CGI script: %s with handler: %s", filePath.c_str(), handlerIt->second.c_str());
  return executeScript(filePath, handlerIt->second, request);
}
void CGIHandler::registerHandler(const std::string &extension,
                                 const std::string &handlerPath) {
  Logger::debugf("Registering CGI handler: %s -> %s", extension.c_str(), handlerPath.c_str());
  _cgi_handlers[extension] = handlerPath;
}
bool CGIHandler::canHandle(const std::string &filePath) const {
  size_t dot_pos = filePath.find_last_of('.');
  if (dot_pos == std::string::npos) {
    Logger::debugf("No extension found in path: %s", filePath.c_str());
    return false;
  }
  std::string extension = filePath.substr(dot_pos);
  bool canHandle = _cgi_handlers.find(extension) != _cgi_handlers.end();
  Logger::debugf("CGI handler check for %s (%s): %s", filePath.c_str(), extension.c_str(), 
                 canHandle ? "YES" : "NO");
  return canHandle;
}
void CGIHandler::setRootDirectory(const std::string &root) {
  Logger::debugf("CGI root directory changed from %s to %s", _root_directory.c_str(), root.c_str());
  _root_directory = root;
}
std::string CGIHandler::executeScript(const std::string &script_path,
                                      const std::string &handler_path,
                                      const HttpParser::Request &request) {
  Logger::warnf("CGI execution not yet implemented for script: %s with handler: %s", 
                script_path.c_str(), handler_path.c_str());
  Logger::debugf("Request method: %s, URI: %s, Body length: %zu", 
                 HTTP::methodToString(request.requestLine.method).c_str(),
                 request.requestLine.uri.c_str(),
                 request.body.length());
  
  // TODO: Implement actual CGI execution using fork/execve as per subject requirements
  // The subject specifically mentions fork can only be used for CGI
  
  return HttpResponseBuilder::createSimpleResponse(HTTP::StatusCode::OK,
                                    "CGI execution not implemented yet");
}
