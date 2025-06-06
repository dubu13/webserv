#include "HTTP/routing/RequestRouter.hpp"
#include "utils/Logger.hpp"
#include "utils/Utils.hpp"

RequestRouter::RequestRouter(const ServerBlock *config) : _config(config) {}

const LocationBlock *RequestRouter::findLocation(const std::string &uri) const {

  if (!_config) {
    Logger::logf<LogLevel::WARN>("No config available, using default location");
    static LocationBlock defaultLocation;
    defaultLocation.path = "/";
    defaultLocation.allowedMethods = {"GET", "POST", "DELETE"};
    return &defaultLocation;
  }

  if (_config->locations.empty()) {
    Logger::logf<LogLevel::INFO>(
        "No location blocks configured, using default location for URI: %s",
        uri.c_str());
    static LocationBlock defaultLocation;
    defaultLocation.path = "/";
    defaultLocation.allowedMethods = {"GET", "POST", "DELETE"};
    return &defaultLocation;
  }

  for (const auto &[path, location] : _config->locations) {
    if (location.path == uri)
      return &location;
  }

  const LocationBlock *bestMatch = nullptr;
  size_t longestMatch = 0;

  for (const auto &[path, location] : _config->locations) {

    if (uri.find(location.path) == 0 && location.path.length() > longestMatch) {

      if (location.path == "/" || location.path.back() == '/' ||
          uri.length() == location.path.length() ||
          uri[location.path.length()] == '/') {
        bestMatch = &location;
        longestMatch = location.path.length();
      }
    }
  }
  if (!bestMatch) {
    for (const auto &[path, location] : _config->locations) {
      if (location.path == "/") {
        Logger::logf<LogLevel::INFO>("Using root location for URI: %s",
                                     uri.c_str());
        return &location;
      }
    }
    Logger::logf<LogLevel::WARN>("No location found for URI: %s, using default",
                                 uri.c_str());
    static LocationBlock defaultLocation;
    defaultLocation.path = "/";
    defaultLocation.allowedMethods = {"GET", "POST", "DELETE"};
    return &defaultLocation;
  }
  Logger::logf<LogLevel::INFO>("Found location for URI %s: %s", uri.c_str(),
                               bestMatch->path.c_str());
  return bestMatch;
}

std::string RequestRouter::resolveRoot(const LocationBlock *location) const {
  if (location && !location->root.empty())
    return location->root;
  if (_config && !_config->root.empty())
    return _config->root;
  return "./www";
}

bool RequestRouter::isMethodAllowed(const Request &request,
                                    const LocationBlock *location) const {
  if (!location || location->allowedMethods.empty())
    return true;
  std::string methodStr = methodToString(request.requestLine.method);
  bool allowed = location->allowedMethods.find(methodStr) !=
                 location->allowedMethods.end();
  return allowed;
}

bool RequestRouter::hasRedirection(const LocationBlock *location) const {
  bool hasRedir = location && !location->redirection.empty();
  return hasRedir;
}

std::string
RequestRouter::getRedirectionTarget(const LocationBlock *location) const {
  if (!location || location->redirection.empty())
    return "";
  return location->redirection;
}
std::string
RequestRouter::handleRedirection(const LocationBlock *location) const {
  if (!hasRedirection(location))
    return "";
  std::string target = getRedirectionTarget(location);
  int code = 302;
  std::string redirectUrl = target;

  size_t spacePos = target.find(' ');
  if (spacePos != std::string::npos) {
    std::string codeStr = target.substr(0, spacePos);
    try {
      int parsedCode = std::stoi(codeStr);
      if (parsedCode == 301 || parsedCode == 302 || parsedCode == 303 ||
          parsedCode == 307 || parsedCode == 308) {
        code = parsedCode;
        redirectUrl = target.substr(spacePos + 1);
      }
    } catch (const std::exception &e) {
      Logger::logf<LogLevel::ERROR>("Failed to parse redirection code: %s",
                                    e.what());
    }
  }
  Logger::logf<LogLevel::INFO>("Performing redirection to %s with code %d",
                               redirectUrl.c_str(), code);
  return HttpResponse::redirect(redirectUrl, code).str();
}
