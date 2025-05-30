#include "config/Config.hpp"
#include "config/ConfigUtils.hpp"
#include <fstream>
#include <sstream>
#include "config/ConfigHandlers.ipp"

Config::Config(const std::string& fileName) : _fileName(fileName) {
    initializeHandlers();
}

void Config::parseFromFile() {
    std::ifstream file(_fileName);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + _fileName);
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    std::vector<std::string> serverBlocks = ConfigUtils::extractServerBlocks(content);
    if (serverBlocks.empty()) {
        throw std::runtime_error("No server blocks found in config file");
    }
    
    for (const auto& blockContent : serverBlocks) {
        ServerBlock server;
        parseServerBlock(blockContent, server);
        
        // Add server with each listen directive as key
        for (const auto& listen : server.listenDirectives) {
            std::string key = listen.first + ":" + std::to_string(listen.second);
            _servers[key] = server;
        }
    }
}

void Config::parseServerBlock(const std::string& content, ServerBlock& server) {
    std::istringstream iss(content);
    std::string line;
    
    while (std::getline(iss, line)) {
        auto [directive, value] = ConfigUtils::parseDirective(line);
        if (directive.empty()) continue;
        
        // Handle location blocks specially
        if (directive.find("location ") == 0) {
            // Extract location path
            std::string locationPath = directive.substr(9);
            if (locationPath.empty() || !ConfigUtils::isValidPath(locationPath)) {
                throw std::invalid_argument("Invalid location path: " + locationPath);
            }
            
            // Extract location block content
            std::string locationContent = line + "\n";
            int braceCount = 1;
            while (std::getline(iss, line) && braceCount > 0) {
                for (char c : line) {
                    if (c == '{') braceCount++;
                    else if (c == '}') braceCount--;
                }
                if (braceCount > 0) {
                    locationContent += line + "\n";
                }
            }
            
            LocationBlock location;
            location.path = locationPath;
            parseLocationBlock(locationContent, location);
            server.locations[locationPath] = location;
            continue;
        }
        
        // Use hash map for directive handling
        auto it = _serverHandlers.find(directive);
        if (it != _serverHandlers.end()) {
            (this->*(it->second))(value, server);
        }
        // Unknown directives are silently ignored for flexibility
    }
    
    // Validate required fields
    if (server.listenDirectives.empty()) {
        throw std::invalid_argument("Server block must have at least one listen directive");
    }
}

void Config::parseLocationBlock(const std::string& content, LocationBlock& location) {
    std::istringstream iss(content);
    std::string line;
    
    while (std::getline(iss, line)) {
        auto [directive, value] = ConfigUtils::parseDirective(line);
        if (directive.empty() || directive.find("location ") == 0) continue;
        
        // Use hash map for directive handling
        auto it = _locationHandlers.find(directive);
        if (it != _locationHandlers.end()) {
            (this->*(it->second))(value, location);
        }
        // Unknown directives are silently ignored for flexibility
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