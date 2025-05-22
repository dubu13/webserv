#pragma once
#include "LocationConfig.hpp"
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include "LocationConfig.hpp"
class ServerConfig {
  private:
      enum class ServerDirective {
        PORT,
        HOST,
        SERVER_NAME,
        ROOT,
        ERROR_PAGES,
        CLIENT_MAX_BODY_SIZE,
        LOCATION,
        UNKNOWN
    };
    
    std::unordered_map<std::string, ServerDirective> _serverDirectives;
  
  public:
      std::string host;
      int port;
      std::string server_name;
      std::string root;
      std::map<int, std::string> error_pages;
      size_t client_max_body_size;
      std::map<std::string, LocationConfig> locations;

      // bool matchesHost(const std::string &host) const;
      // const LocationConfig *getLocation(const std::string &path) const;
      
      ServerConfig();
      void parseServerBlock(std::ifstream &file);
      void handleServerDirective(ServerDirective type, std::istringstream& iss);
};
