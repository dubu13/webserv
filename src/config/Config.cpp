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
            std::string key = serverBlock.host + ":" + std::to_string(serverBlock.port);
            _servers[key] = serverBlock;
        }
        else {
            file.close();
            throw std::runtime_error("Invalid config format, expected Server Block");
        }
    }
    file.close();
    return _servers;
}