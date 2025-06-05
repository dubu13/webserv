#pragma once
#include <vector>
#include <string>
#include <map>
#include "config/LocationBlock.hpp"

struct ServerBlock {
    std::vector<std::pair<std::string, int>> listenDirectives;
    std::string host;
    std::vector<std::string> serverNames;
    std::string root;
    std::string index;
    std::map<int, std::string> errorPages;
    size_t clientMaxBodySize;
    std::map<std::string, LocationBlock> locations;

    ServerBlock()
        : host("0.0.0.0"), clientMaxBodySize(1024 * 1024) {
    }

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

    const LocationBlock* getLocation(const std::string& path) const;
    
private:
    const LocationBlock* findBestLocationMatch(const std::string& path) const;
};
