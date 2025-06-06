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
  FileCache(size_t maxEntries = 100) : maxEntries(maxEntries) {}

  bool getFile(const std::string &path, std::string &content,
               std::string &mimeType) {
    auto it = cache.find(path);
    if (it != cache.end()) {
      content = it->second.content;
      mimeType = it->second.mimeType;
      return true;
    }
    return false;
  }

  void cacheFile(const std::string &path, const std::string &content,
                 const std::string &mimeType) {

    if (cache.size() >= maxEntries && cache.find(path) == cache.end()) {
      auto it = cache.begin();
      cache.erase(it);
    }

    cache[path] = {content, mimeType};
  }

  void clearCache() { cache.clear(); }
};
