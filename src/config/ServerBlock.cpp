#include "config/ServerBlock.hpp"
#include "utils/Utils.hpp"

const LocationBlock* ServerBlock::getLocation(const std::string& path) const {
    return findBestLocationMatch(path);
}

const LocationBlock* ServerBlock::findBestLocationMatch(const std::string& path) const {
    std::string cleanPath = HttpUtils::sanitizePath(path);
    
    // Handle root path specially
    if (cleanPath == "/") {
        auto it = locations.find("/");
        if (it != locations.end()) {
            return &it->second;
        }
        return nullptr;
    }
    
    const LocationBlock* bestMatch = nullptr;
    size_t bestMatchLength = 0;
    
    for (const auto& [prefix, location] : locations) {
        // Skip root location for non-root paths
        if (prefix == "/" && cleanPath != "/") {
            continue;
        }
        
        // Check if path starts with this location prefix
        if (cleanPath.find(prefix) == 0) {
            // Ensure we don't match partial segments (e.g., /api shouldn't match /apitest)
            if (prefix.length() > 1 &&
                cleanPath.length() > prefix.length() &&
                cleanPath[prefix.length()] != '/') {
                continue;
            }
            
            // Track the longest matching prefix
            if (prefix.length() > bestMatchLength) {
                bestMatch = &location;
                bestMatchLength = prefix.length();
            }
        }
    }
    
    return bestMatch;
}
