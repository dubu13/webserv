#pragma once
#include "ServerConfig.hpp"
#include <unordered_map>
#include <string>
#include <sstream>
#include <fstream>

class Config {
  private:

      std::string _fileName;
      std::unordered_map<std::string, ServerConfig> _servers;

  public:
      Config(const std::string &fileName);
      std::unordered_map<std::string, ServerConfig>& parseConfig();
    // const ServerConfig &resolveServer(const std::string &host, int port,
    //                                   const std::string &hostHeader) const;
};
