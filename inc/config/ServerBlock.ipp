#pragma once
#include <vector>
#include <string>
#include <map>

struct LocationBlock;

struct ServerBlock {
    std::vector<std::pair<std::string, int>> listenDirectives;
    std::string host;
    std::vector<std::string> serverNames;
    std::string root;
    std::string index;
    std::map<int, std::string> errorPages;
    size_t clientMaxBodySize;
    std::map<std::string, LocationBlock> locations;
    
    ServerBlock();
    bool matchesHost(const std::string& host) const;
    const LocationBlock* getLocation(const std::string& path) const;
};
