#pragma once
#include "HTTP/HTTPTypes.hpp"
#include <string>
#include <unordered_map>
#include <chrono>

namespace FileUtils {
  // Simple file cache for performance
  struct CacheEntry {
    std::string content;
    std::chrono::steady_clock::time_point timestamp;
    size_t size;
  };
  
  void clearCache();
  void setCacheMaxSize(size_t maxSize);
  
  // Existing functions
  std::string readFile(const std::string &rootDir, const std::string &uri, HTTP::StatusCode &status);
  bool writeFile(const std::string &rootDir, const std::string &uri, const std::string &content, HTTP::StatusCode &status);
  bool deleteFile(const std::string &rootDir, const std::string &uri, HTTP::StatusCode &status);
  bool fileExists(const std::string &rootDir, const std::string &uri, size_t &size);
  
  std::string buildPath(const std::string &root, const std::string &path);
  bool isPathSafe(const std::string &path);
  std::string sanitizePath(const std::string &path);
  
  std::string readFileContent(const std::string &filePath);
  bool writeFileContent(const std::string &filePath, const std::string &content);
}
