#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
    : host("localhost"), port(8080), root("./www"),
      client_max_body_size(1024 * 1024) {} // 1 MB default

bool ServerConfig::matchesHost(const std::string &host) const {
  if (server_names.empty()) {
    return true; // If no server names are specified, match all
  }
  return std::find(server_names.begin(), server_names.end(), host) !=
         server_names.end();
}

const LocationConfig *ServerConfig::getLocation(const std::string &path) const {
  for (const auto &location : locations) {
    if (path.find(location.first) == 0) {
      return &location.second;
    }
  }
  return nullptr; // No matching location found
}
