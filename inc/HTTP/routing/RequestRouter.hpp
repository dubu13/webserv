#pragma once

#include "HTTP/core/HTTPParser.hpp"
#include "HTTP/core/HTTPTypes.hpp"
#include "HTTP/core/HttpResponse.hpp"
#include "config/LocationBlock.hpp"
#include "config/ServerBlock.hpp"
#include <string>
#include <string_view>

using HTTP::methodToString;
using HTTP::Request;

class RequestRouter {
private:
  const ServerBlock *_config;

public:
  explicit RequestRouter(const ServerBlock *config);
  const LocationBlock *findLocation(const std::string &uri) const;
  std::string resolveRoot(const LocationBlock *location) const;
  bool isMethodAllowed(const Request &request,
                       const LocationBlock *location) const;
  bool hasRedirection(const LocationBlock *location) const;
  std::string getRedirectionTarget(const LocationBlock *location) const;
  std::string handleRedirection(const LocationBlock *location) const;
  std::string getIndexFile(const LocationBlock *location) const;
  std::string getRelativePath(const std::string &uri, const LocationBlock *location) const;
  const ServerBlock *getConfig() const { return _config; }
};
