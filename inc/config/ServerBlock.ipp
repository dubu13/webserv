#pragma once
#include <vector>
#include <string>
#include <map>
#include "config/LocationBlock.ipp"

struct ServerBlock {
    std::vector<std::pair<std::string, int>> listenDirectives;
    std::string host;
    std::vector<std::string> serverNames;
    std::string root;
    std::string index;
    std::map<int, std::string> errorPages;
    size_t clientMaxBodySize;
    std::map<std::string, LocationBlock> locations;
    
    // Inline constructor
    ServerBlock() 
        : host("0.0.0.0"), clientMaxBodySize(1024 * 1024) {
    }
    
    // Inline host matching method
    bool matchesHost(const std::string& requestHost) const {
        if (serverNames.empty()) return true;
        
        for (const auto& name : serverNames) {
            if (name == "*" || name == requestHost) return true;
            if (name.size() > 2 && name.substr(0, 2) == "*." && 
                requestHost.size() >= name.size() - 1) {
                std::string suffix = requestHost.substr(requestHost.size() - (name.size() - 1));
                if (suffix == name.substr(2)) return true;
            }
        }
        return false;
    }
    
    // Inline location matching method
    const LocationBlock* getLocation(const std::string& path) const {
        std::string bestMatch = "";
        for (const auto& [locationPath, config] : locations) {
            if (locationPath == "/" || 
                (path.find(locationPath) == 0 && 
                 (path.length() == locationPath.length() || 
                  path[locationPath.length()] == '/'))) {
                if (locationPath.length() > bestMatch.length()) {
                    bestMatch = locationPath;
                }
            }
        }
        return bestMatch.empty() ? nullptr : &locations.at(bestMatch);
    }
};
