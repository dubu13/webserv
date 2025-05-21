#include "Config.hpp"

Config::Config(const std::string &fileName) : _fileName(fileName) {}

std::unordered_map<std::string, ServerConfig>& Config::parseConfig() {
    std::ifstream file(_fileName);
    if (!file.is_open())
        throw std::runtime_error("Could not open config file");

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        if (line == "server {") {
            ServerConfig server;
            parseServerBlock(file, server);
            std::string key = server.host + ":" + std::to_string(server.port);
            _servers[key] = server;
        }
        else {
            file.close();
            throw std::runtime_error("Invalid config format, expected Server Block");
        }
    }
    file.close();
    return _servers;
}

void Config::parseServerBlock(std::ifstream &file, ServerConfig &server) {
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        if (line == "}")
            return;
         std::ifstream iss(line);
         std::string directive;
         iss >> directive;

         if (directive == "location") {
            LocationConfig location;
            parseLocationBlock(file, location);
         }
        //     hadnle other directives
    }
}

void Config::parseLocationBlock(std::ifstream &file, LocationConfig &location) {
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        if (line == "}")
            return;
        // Parse location directives
    }
}