#include "HTTP/routing/RequestRouter.hpp"
#include "utils/Logger.hpp"
#include "utils/Utils.hpp"

RequestRouter::RequestRouter(const ServerBlock *config) : _config(config) {}

const LocationBlock *RequestRouter::findLocation(const std::string &uri) const {

  std::string cleanUri = HttpUtils::cleanUri(uri);

  if (!_config) {
    Logger::logf<LogLevel::WARN>("No config available, using default location");
    static LocationBlock defaultLocation;
    defaultLocation.path = "/";
    defaultLocation.allowedMethods = {"GET", "POST", "DELETE"};
    return &defaultLocation;
  }

  if (_config->locations.empty())
    Logger::logf<LogLevel::INFO>(
        "No location blocks configured, using default location for URI: %s",
        cleanUri.c_str());
  for (const auto &[path, location] : _config->locations) {
    if (cleanUri == path) {
      Logger::logf<LogLevel::INFO>("Found exact location match for URI %s: %s", cleanUri.c_str(), path.c_str());
      return &location;
    }
  }

  const LocationBlock *bestMatch = nullptr;
  size_t longestMatch = 0;

  for (const auto &[path, location] : _config->locations) {

    bool isValidPrefix = false;
    if (cleanUri.length() >= path.length()) {
      if (cleanUri.substr(0, path.length()) == path) {

        if (cleanUri.length() == path.length() ||
            cleanUri[path.length()] == '/' ||
            path.back() == '/') {
          isValidPrefix = true;
        }
      }
    }

    if (isValidPrefix && path.length() > longestMatch) {
      bestMatch = &location;
      longestMatch = path.length();
    }
  }

  if (!bestMatch) {

    static LocationBlock defaultLocation;
    defaultLocation.path = "/";
    defaultLocation.allowedMethods = {"GET", "POST", "DELETE"};
    bestMatch = &defaultLocation;
  }

  Logger::logf<LogLevel::INFO>("Found location for URI %s: %s", cleanUri.c_str(),
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
  if (!location || location->allowedMethods.empty()) {
    return true;
  }

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

    try {
      int parsedCode = std::stoi(target.substr(0, spacePos));

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

std::string RequestRouter::getIndexFile(const LocationBlock *location) const {

  if (location && !location->index.empty())
    return location->index;
  if (_config && !_config->index.empty())
    return _config->index;
  return "index.html";
}

std::string RequestRouter::getRelativePath(const std::string &uri, const LocationBlock *location) const {

  std::string cleanUri = HttpUtils::cleanUri(uri);
  if (!location || location->path.empty() || location->path == "/") 
    return cleanUri;

  if (cleanUri.length() >= location->path.length() &&
      cleanUri.substr(0, location->path.length()) == location->path) {
    std::string relativePath = cleanUri.substr(location->path.length());
    if (relativePath.empty() || relativePath[0] != '/')
      relativePath = "/" + relativePath;
    return relativePath;
  }
  return cleanUri;
}
