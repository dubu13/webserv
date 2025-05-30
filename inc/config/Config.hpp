#pragma once
#include "ServerConfig.hpp"
#include <fstream>
#include <string>
#include <unordered_map>
#include <stdexcept>

class Config {
private:
    std::string _fileName;
    std::unordered_map<std::string, ServerConfig> _servers;

public:
    Config(const std::string &fileName);
    std::unordered_map<std::string, ServerConfig>& parseConfig();
};
