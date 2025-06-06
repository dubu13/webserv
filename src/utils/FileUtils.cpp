#include "utils/Constants.hpp"
#include "utils/FileCache.ipp"
#include "utils/Logger.hpp"
#include "utils/Utils.hpp"
#include "utils/ValidationUtils.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using HTTP::StatusCode;

static FileCache fileCache(Constants::DEFAULT_CACHE_SIZE);

static std::string readFileFromPath(const std::string &filePath,
                                    StatusCode &status) {
  std::string content, mimeType;
  if (fileCache.getFile(filePath, content, mimeType)) {
    status = StatusCode::OK;
    return content;
  }

  if (!FileUtils::exists(filePath)) {
    status = StatusCode::NOT_FOUND;
    return "";
  }

  std::ifstream file(filePath, std::ios::binary);
  if (!file.is_open()) {
    status = StatusCode::NOT_FOUND;
    return "";
  }

  content = std::string((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
  file.close();

  fileCache.cacheFile(filePath, content, FileUtils::getMimeType(filePath));
  status = StatusCode::OK;
  return content;
}

std::string FileUtils::readFile(std::string_view rootDir, std::string_view uri,
                                StatusCode &status) {
  if (!rootDir.empty()) {
    auto validation = ValidationUtils::validateFileAccess(rootDir, uri);
    if (!validation.isValid) {
      status = validation.status;
      return "";
    }
    return readFileFromPath(validation.filePath, status);
  } else {
    return readFileFromPath(std::string(uri), status);
  }
}

bool FileUtils::writeFile(std::string_view rootDir, std::string_view uri,
                          std::string_view content, StatusCode &status) {
  auto validation = ValidationUtils::validateFileAccess(rootDir, uri);
  if (!validation.isValid) {
    status = validation.status;
    return false;
  }

  if (writeFileContent(validation.filePath, content)) {
    fileCache.cacheFile(validation.filePath, std::string(content), "text/html");
    status = StatusCode::CREATED;
    return true;
  }

  status = StatusCode::INTERNAL_SERVER_ERROR;
  return false;
}

bool FileUtils::deleteFile(std::string_view rootDir, std::string_view uri,
                           StatusCode &status) {
  auto validation = ValidationUtils::validateFileAccess(rootDir, uri);
  if (!validation.isValid) {
    status = validation.status;
    return false;
  }

  struct stat fileStat;
  if (stat(validation.filePath.c_str(), &fileStat) != 0) {
    status = StatusCode::NOT_FOUND;
    return false;
  }

  if (S_ISDIR(fileStat.st_mode)) {
    status = StatusCode::FORBIDDEN;
    return false;
  }

  if (unlink(validation.filePath.c_str()) == 0) {
    status = StatusCode::NO_CONTENT;
    return true;
  }

  status = StatusCode::INTERNAL_SERVER_ERROR;
  return false;
}

bool FileUtils::isDirectory(std::string_view path) {
  struct stat pathStat;
  return stat(std::string(path).c_str(), &pathStat) == 0 &&
         S_ISDIR(pathStat.st_mode);
}

std::string FileUtils::generateDirectoryListing(std::string_view dirPath,
                                                std::string_view uri) {
  std::string html = "<html><head><title>Directory listing for " +
                     std::string(uri) +
                     "</title>"
                     "<style>body{font-family:Arial,sans-serif;margin:20px}h1{"
                     "color:#333;border-bottom:1px solid #ccc}"
                     "ul{list-style-type:none;padding:0}li{margin:5px "
                     "0}a{text-decoration:none;color:#0066cc}"
                     "a:hover{text-decoration:underline}.dir{font-weight:bold}."
                     "file{color:#666}</style></head><body>"
                     "<h1>Directory listing for " +
                     std::string(uri) + "</h1><hr><ul>";

  if (uri != "/") {
    html += "<li><a href=\"../\" class=\"dir\">../</a></li>";
  }

  DIR *dir = opendir(std::string(dirPath).c_str());
  if (!dir) {
    html += "</ul><hr><em>Error reading directory</em></body></html>";
    return html;
  }

  std::vector<std::pair<std::string, bool>> entries;
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    std::string name = entry->d_name;
    if (name == "." || name == "..")
      continue;

    std::string fullPath = HttpUtils::buildPath(dirPath, name);
    entries.emplace_back(name, FileUtils::isDirectory(fullPath));
  }
  closedir(dir);

  std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
    return a.second != b.second ? a.second > b.second : a.first < b.first;
  });

  for (const auto &[name, isDir] : entries) {
    const std::string href = name + (isDir ? "/" : "");
    const std::string cssClass = isDir ? "dir" : "file";
    const std::string displayName = name + (isDir ? "/" : "");
    html += "<li><a href=\"" + href + "\" class=\"" + cssClass + "\">" +
            displayName + "</a></li>";
  }

  html += "</ul><hr><em>Generated by WebServ</em></body></html>";
  return html;
}

std::optional<std::string>
FileUtils::readFileContent(std::string_view filePath) {
  std::ifstream file(std::string(filePath), std::ios::binary);
  return file.is_open() ? std::make_optional(std::string(
                              (std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>()))
                        : std::nullopt;
}

bool FileUtils::writeFileContent(std::string_view filePath,
                                 std::string_view content) {
  std::ofstream file(std::string(filePath), std::ios::binary);
  if (!file.is_open())
    return false;

  file.write(content.data(), content.size());
  file.close();
  return file.good();
}

void FileUtils::clearCache() { fileCache.clearCache(); }

void FileUtils::setCacheMaxSize(size_t maxSize) {
  fileCache = FileCache(maxSize);
}

bool FileUtils::exists(std::string_view path) {
  struct stat fileStat;
  return stat(std::string(path).c_str(), &fileStat) == 0;
}

bool FileUtils::createDirectories(std::string_view path) {
  try {
    return std::filesystem::create_directories(std::filesystem::path(path));
  } catch (const std::exception &) {
    return false;
  }
}

std::string FileUtils::getMimeType(std::string_view filePath) {
  size_t dotPos = filePath.find_last_of('.');
  if (dotPos == std::string::npos || dotPos == filePath.length() - 1) {
    return "application/octet-stream";
  }

  std::string extension = std::string(filePath.substr(dotPos + 1));
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 ::tolower);

  static const std::unordered_map<std::string_view, std::string_view>
      extensionToMimeMap = {{"html", "text/html; charset=utf-8"},
                            {"htm", "text/html; charset=utf-8"},
                            {"css", "text/css"},
                            {"js", "application/javascript"},
                            {"json", "application/json"},
                            {"txt", "text/plain; charset=utf-8"},
                            {"png", "image/png"},
                            {"jpg", "image/jpeg"},
                            {"jpeg", "image/jpeg"},
                            {"gif", "image/gif"},
                            {"pdf", "application/pdf"},
                            {"ico", "image/x-icon"},
                            {"svg", "image/svg+xml"},
                            {"xml", "application/xml"},
                            {"zip", "application/zip"},
                            {"mp3", "audio/mpeg"},
                            {"mp4", "video/mp4"}};

  auto it = extensionToMimeMap.find(extension);
  return (it != extensionToMimeMap.end()) ? std::string(it->second)
                                          : "application/octet-stream";
}
