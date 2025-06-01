#pragma once
#include "HTTP/HTTPTypes.ipp"
#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <chrono>
#include <functional>

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
  std::string readFile(std::string_view rootDir, std::string_view uri, HTTP::StatusCode &status);
  bool writeFile(std::string_view rootDir, std::string_view uri, std::string_view content, HTTP::StatusCode &status);
  bool deleteFile(std::string_view rootDir, std::string_view uri, HTTP::StatusCode &status);
  std::optional<size_t> fileExists(std::string_view rootDir, std::string_view uri);
  
  // Directory utilities
  bool isDirectory(std::string_view path);
  std::string generateDirectoryListing(std::string_view dirPath, std::string_view uri);
  
  // Path utilities
  std::string extractQueryParams(std::string_view uri);
  std::string cleanUri(std::string_view uri);
  
  std::string buildPath(std::string_view root, std::string_view path);
  bool isPathSafe(std::string_view path);
  std::string sanitizePath(std::string_view path);
  
  std::optional<std::string> readFileContent(std::string_view filePath);
  bool writeFileContent(std::string_view filePath, std::string_view content);
}
