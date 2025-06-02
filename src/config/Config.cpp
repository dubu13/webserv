#include "config/Config.hpp"
#include "utils/Logger.hpp"
#include "config/ConfigUtils.hpp"
#include <fstream>
#include <sstream>

Config::Config(const std::string& fileName) : _fileName(fileName) {
    Logger::infof("Config constructor with file: %s", fileName.c_str());
    initializeHandlers();
}

void Config::parseFromFile() {
    Logger::infof("Starting to parse config file: %s", _fileName.c_str());
    std::ifstream file(_fileName);
    if (!file.is_open()) {
        Logger::errorf("Could not open config file: %s", _fileName.c_str());
        throw std::runtime_error("Could not open config file: " + _fileName);
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    Logger::debugf("Read %zu bytes from config file", content.length());

    std::vector<std::string> serverBlocks = ConfigUtils::extractServerBlocks(content);
    if (serverBlocks.empty()) {
        Logger::error("No server blocks found in config file");
        throw std::runtime_error("No server blocks found in config file");
    }

    Logger::infof("Found %zu server blocks in config", serverBlocks.size());

    for (size_t i = 0; i < serverBlocks.size(); ++i) {
        Logger::debugf("Parsing server block %zu...", i + 1);
        ServerBlock server;
        parseServerBlock(serverBlocks[i], server);

        for (const auto& listen : server.listenDirectives) {
            std::string hostToUse = server.host.empty() ? listen.first : server.host;
            std::string key = hostToUse + ":" + std::to_string(listen.second);
            _servers[key] = server;
            Logger::infof("Added server configuration for %s", key.c_str());
        }
    }

    Logger::infof("Configuration parsing completed successfully. Total servers: %zu", _servers.size());
}

void Config::parseServerBlock(const std::string& content, ServerBlock& server) {
    Logger::debug("Parsing server block...");
    std::istringstream iss(content);
    std::string line;

    while (std::getline(iss, line)) {
        auto [directive, value] = ConfigUtils::parseDirective(line);
        if (directive.empty()) continue;

        Logger::debugf("Processing directive: '%s' with value: '%s'", directive.c_str(), value.c_str());

        if (directive == "location") {
            std::string locationPath = value;

            size_t bracePos = locationPath.find('{');
            if (bracePos != std::string::npos)
                locationPath = locationPath.substr(0, bracePos);
            locationPath = ConfigUtils::trim(locationPath);

            if (locationPath.empty() || !ConfigUtils::isValidPath(locationPath))
                throw std::invalid_argument("Invalid location path: " + locationPath);

            std::string locationContent = line + "\n";
            int braceCount = 1;
            while (std::getline(iss, line) && braceCount > 0) {
                for (char c : line) {
                    if (c == '{') braceCount++;
                    else if (c == '}') braceCount--;
                }
                if (braceCount > 0)
                    locationContent += line + "\n";
            }

            LocationBlock location;
            location.path = locationPath;
            Logger::debugf("Parsing location block for path: %s", locationPath.c_str());
            parseLocationBlock(locationContent, location);
            server.locations[locationPath] = location;
            Logger::debugf("Added location %s with root: %s", locationPath.c_str(), location.root.c_str());
            continue;
        }
        auto it = _serverHandlers.find(directive);
        if (it != _serverHandlers.end()) {
            Logger::debugf("Handling server directive: %s = %s", directive.c_str(), value.c_str());
            (this->*(it->second))(value, server);
        } else {
            Logger::warnf("Unknown server directive: %s", directive.c_str());
        }

    }

    Logger::debugf("Server block parsed. Root: %s, Locations: %zu",
                   server.root.c_str(), server.locations.size());
    if (server.listenDirectives.empty())
        throw std::invalid_argument("Server block must have at least one listen directive");
}

void Config::parseLocationBlock(const std::string& content, LocationBlock& location) {
    Logger::debugf("Parsing location block content: %s", content.c_str());
    std::istringstream iss(content);
    std::string line;

    while (std::getline(iss, line)) {
        auto [directive, value] = ConfigUtils::parseDirective(line);
        if (directive.empty() || directive.find("location ") == 0) continue;

        Logger::debugf("Location directive: '%s' = '%s'", directive.c_str(), value.c_str());

        auto it = _locationHandlers.find(directive);
        if (it != _locationHandlers.end()) {
            (this->*(it->second))(value, location);
        }

    }
}

const std::unordered_map<std::string, ServerBlock>& Config::getServers() const {
    return _servers;
}

const ServerBlock* Config::getServer(const std::string& host, int port) const {
    std::string key = host + ":" + std::to_string(port);
    auto it = _servers.find(key);
    return (it != _servers.end()) ? &it->second : nullptr;
}
