#pragma once
#include "HTTP/HTTPTypes.hpp"
#include <string>
namespace FileOps {
std::string readFile(const std::string &rootDir, const std::string &uri,
                     HTTP::StatusCode &status);
bool writeFile(const std::string &rootDir, const std::string &uri,
               const std::string &content, HTTP::StatusCode &status);
bool deleteFile(const std::string &rootDir, const std::string &uri,
                HTTP::StatusCode &status);
bool fileExists(const std::string &rootDir, const std::string &uri,
                size_t &size);
std::string getMimeType(const std::string &path);
std::string buildPath(const std::string &rootDir, const std::string &uri);
bool isPathSafe(const std::string &uri);
} // namespace FileOps
