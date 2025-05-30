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
    FileCache(size_t maxEntries = 100);
    
    bool getFile(const std::string& path, std::string& content, std::string& mimeType);
    void cacheFile(const std::string& path, const std::string& content, const std::string& mimeType);
    void clearCache();
};