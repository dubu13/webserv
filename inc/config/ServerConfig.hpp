#pragma once
#include "LocationConfig.hpp"
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <arpa/inet.h>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <utility>
#include "LocationConfig.hpp"
class ServerConfig {
  private:
      enum class ServerDirective {
        LISTEN,
        HOST,
        SERVER_NAME,
        ROOT,
        INDEX,
        ERROR_PAGES,
        CLIENT_MAX_BODY_SIZE,
        LOCATION,
        UNKNOWN
    };
    
    std::unordered_map<std::string, ServerDirective> _serverDirectives;
    size_t _maxAllowedBodySize;
  public:
      std::vector<std::pair<std::string, int>> listenDirectives;
      std::string host;
      std::vector<std::string> serverNames;
      std::string root;
      std::optional<std::string> index_file;
      std::map<int, std::string> error_pages;
      size_t client_max_body_size;
      std::map<std::string, LocationConfig> locations;

      ServerConfig();
      
      bool matchesHost(const std::string &host) const;
      const LocationConfig *getLocation(const std::string &path) const;
      void handleListen(std::string value);
      size_t handleBodySize(std::string& value); 
      void parseServerBlock(std::ifstream &file);
      void handleServerDirective(ServerDirective type, std::istringstream& iss);
      void validateConfig();
      bool validateIPv4(const std::string &ip) const;
      bool validateServerName(const std::string& name) const;
      bool isValidWildcard(const std::string& name) const;
      bool isLabelValid(const std::string& label) const;
};
