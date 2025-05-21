#pragma once
#include "ServerConfig.hpp"
#include <unordered_map>
#include <string>
#include <fstream>

class Config {
  private:
      std::string _fileName;
      std::unordered_map<std::string, ServerConfig> _servers;
      void parseServerBlock(std::ifstream &file, ServerConfig &server);
      void parseLocationBlock(std::ifstream &file, LocationConfig &location);
  public:
      Config(const std::string &fileName);
      std::unordered_map<std::string, ServerConfig>& parseConfig();
    // const ServerConfig &resolveServer(const std::string &host, int port,
    //                                   const std::string &hostHeader) const;
};
