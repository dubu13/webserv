#include "HTTP/core/ErrorResponseBuilder.hpp"
#include "HTTP/core/HTTPTypes.hpp"
#include "HTTP/core/HttpResponse.hpp"
#include "utils/Utils.hpp"
#include <fstream>
#include <sstream>

const ServerBlock *ErrorResponseBuilder::_currentConfig = nullptr;

void ErrorResponseBuilder::setCurrentConfig(const ServerBlock *config) {
  _currentConfig = config;
}

std::string ErrorResponseBuilder::buildResponse(int statusCode) {
  std::string customPage = loadCustomErrorPage(statusCode);
  if (!customPage.empty())
    return HttpResponse::buildResponse(
        statusCode, HTTP::statusToString(statusCode), customPage, "text/html");
  return buildDefaultError(statusCode);
}

std::string ErrorResponseBuilder::loadCustomErrorPage(int statusCode) {
  if (!_currentConfig || _currentConfig->errorPages.find(statusCode) ==
                             _currentConfig->errorPages.end())
    return "";
  std::string errorPagePath =
      _currentConfig->root + "/" + _currentConfig->errorPages.at(statusCode);
  std::ifstream file(errorPagePath);
  if (!file.is_open())
    return "";
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

std::string ErrorResponseBuilder::buildDefaultError(int statusCode) {
  std::string statusText = HTTP::statusToString(statusCode);
  std::ostringstream content;
  content << "<html><body><h1>" << statusCode << " " << statusText
          << "</h1></body></html>";
  return HttpResponse::buildResponse(statusCode, statusText, content.str(),
                                     "text/html");
}
