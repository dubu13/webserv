#include "Config.hpp"

Config::Config(const std::string &fileName) : _fileName(fileName) {
}

std::unordered_map<std::string, ServerConfig>& Config::parseConfig() {
    std::ifstream file(_fileName);
    if (!file.is_open())
        throw std::runtime_error("Could not open config file");

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        if (line == "server {") {
            ServerConfig serverBlock;
            serverBlock.parseServerBlock(file);
            for (const auto& directive : serverBlock.listenDirectives) {
                std::string key = directive.first + ":" + std::to_string(directive.second);
                _servers[key] = serverBlock;
            }
        }
        else {
            file.close();
            throw std::runtime_error("Invalid config format, expected Server Block");
        }
    }
    file.close();
    if (_servers.empty())
        throw std::runtime_error("No valid server blocks found in config file");
    return _servers;
}