#pragma once

#include <string>
#include <unordered_map>

class FileCache {
private:
    struct CacheEntry {
        std::string content;
        std::string mimeType;
    };
    
    std::unordered_map<std::string, CacheEntry> cache;
    size_t maxEntries;

public:
    // Inline constructor
    FileCache(size_t maxEntries = 100) : maxEntries(maxEntries) {
    }
    
    // Inline cache retrieval
    bool getFile(const std::string& path, std::string& content, std::string& mimeType) {
        auto it = cache.find(path);
        if (it != cache.end()) {
            content = it->second.content;
            mimeType = it->second.mimeType;
            return true;
        }
        return false;
    }
    
    // Inline cache storage with LRU eviction
    void cacheFile(const std::string& path, const std::string& content, const std::string& mimeType) {
        // If at max capacity, remove oldest entry (LRU approximation)
        if (cache.size() >= maxEntries && cache.find(path) == cache.end()) {
            // Remove first entry as approximation of LRU
            // In a full implementation, we'd track access order
            auto it = cache.begin();
            cache.erase(it);
        }
        
        cache[path] = {content, mimeType};
    }
    
    // Inline cache clearing
    void clearCache() {
        cache.clear();
    }
};