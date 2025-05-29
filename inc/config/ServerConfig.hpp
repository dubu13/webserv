#pragma once
#include "LocationConfig.hpp"
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
class ServerConfig {
public:
  std::string host;
  int port;
  std::vector<std::string> server_name;
  std::string root;
  std::map<int, std::string> error_pages;
  size_t client_max_body_size;
  std::map<std::string, LocationConfig> locations;
  ServerConfig();
  bool matchesHost(const std::string &host) const;
  const LocationConfig *getLocation(const std::string &path) const;
  void parseServerBlock(std::ifstream &file);
};
