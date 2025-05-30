#include "utils/FileCache.hpp"

FileCache::FileCache(size_t maxEntries) : maxEntries(maxEntries) {
}

bool FileCache::getFile(const std::string& path, std::string& content, std::string& mimeType) {
    auto it = cache.find(path);
    if (it != cache.end()) {
        content = it->second.content;
        mimeType = it->second.mimeType;
        return true;
    }
    return false;
}

void FileCache::cacheFile(const std::string& path, const std::string& content, const std::string& mimeType) {
    // Simple eviction: if we're at max capacity, clear the cache
    if (cache.size() >= maxEntries) {
        cache.clear();
    }
    
    cache[path] = {content, mimeType};
}

void FileCache::clearCache() {
    cache.clear();
}