#include "LocationConfig.hpp"

LocationConfig::LocationConfig()
    : root(), autoindex(false), allow_upload(false) {}

bool LocationConfig::isMethodAllowed(const std::string &method) const {
  if (allowed_methods.empty()) {
    return true; // If no methods are specified, allow all
  }
  return allowed_methods.find(method) != allowed_methods.end();
}

bool LocationConfig::isCGI(const std::string &path) const {
  if (cgi_extension) {
    return path.find(*cgi_extension) != std::string::npos;
  }
  return false;
}
