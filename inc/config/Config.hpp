#pragma once
#include "ServerConfig.hpp"
#include <vector>

class Config {
public:
  std::vector<ServerConfig> servers;

  const ServerConfig &resolveServer(const std::string &host, int port,
                                    const std::string &hostHeader) const;
};
