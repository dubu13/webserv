#include "resource/FileHandler.hpp"
#include <filesystem>
#include <fstream>
namespace FileOps {
std::string buildPath(const std::string &rootDir, const std::string &uri) {
  std::string normalizedUri = (uri == "/") ? "/index.html" : uri;
  return rootDir + normalizedUri;
}
bool isPathSafe(const std::string &uri) {
  return uri.find("..") == std::string::npos;
}
std::string getMimeType(const std::string &path) {
  size_t dotPos = path.find_last_of('.');
  if (dotPos == std::string::npos)
    return "application/octet-stream";
  std::string ext = path.substr(dotPos + 1);
  if (ext == "html" || ext == "htm")
    return "text/html";
  if (ext == "css")
    return "text/css";
  if (ext == "js")
    return "text/javascript";
  if (ext == "json")
    return "application/json";
  if (ext == "txt")
    return "text/plain";
  if (ext == "png")
    return "image/png";
  if (ext == "jpg" || ext == "jpeg")
    return "image/jpeg";
  if (ext == "gif")
    return "image/gif";
  if (ext == "pdf")
    return "application/pdf";
  return "application/octet-stream";
}
std::string readFile(const std::string &rootDir, const std::string &uri,
                     HTTP::StatusCode &status) {
  if (!isPathSafe(uri)) {
    status = HTTP::StatusCode::FORBIDDEN;
    return "";
  }
  std::string filePath = buildPath(rootDir, uri);
  std::ifstream file(filePath, std::ios::binary);
  if (!file.is_open()) {
    status = HTTP::StatusCode::NOT_FOUND;
    return "";
  }
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  status = HTTP::StatusCode::OK;
  return content;
}
bool writeFile(const std::string &rootDir, const std::string &uri,
               const std::string &content, HTTP::StatusCode &status) {
  if (!isPathSafe(uri)) {
    status = HTTP::StatusCode::FORBIDDEN;
    return false;
  }
  std::string filePath = buildPath(rootDir, uri);
  try {
    std::filesystem::create_directories(
        std::filesystem::path(filePath).parent_path());
  } catch (...) {
    status = HTTP::StatusCode::INTERNAL_SERVER_ERROR;
    return false;
  }
  bool existed = std::filesystem::exists(filePath);
  std::ofstream file(filePath, std::ios::binary);
  if (!file.is_open() || !(file << content)) {
    status = HTTP::StatusCode::INTERNAL_SERVER_ERROR;
    return false;
  }
  status = existed ? HTTP::StatusCode::OK : HTTP::StatusCode::CREATED;
  return true;
}
bool deleteFile(const std::string &rootDir, const std::string &uri,
                HTTP::StatusCode &status) {
  if (!isPathSafe(uri) || uri == "/" || uri == "/index.html" ||
      uri.find("/errors/") == 0) {
    status = HTTP::StatusCode::FORBIDDEN;
    return false;
  }
  std::string filePath = buildPath(rootDir, uri);
  if (!std::filesystem::exists(filePath)) {
    status = HTTP::StatusCode::NOT_FOUND;
    return false;
  }
  if (std::filesystem::remove(filePath)) {
    status = HTTP::StatusCode::NO_CONTENT;
    return true;
  }
  status = HTTP::StatusCode::INTERNAL_SERVER_ERROR;
  return false;
}
bool fileExists(const std::string &rootDir, const std::string &uri,
                size_t &size) {
  if (!isPathSafe(uri))
    return false;
  std::string filePath = buildPath(rootDir, uri);
  if (!std::filesystem::exists(filePath))
    return false;
  try {
    size = std::filesystem::file_size(filePath);
  } catch (...) {
    size = 0;
  }
  return true;
}
} // namespace FileOps
