#include "utils/FileUtils.hpp"
#include "utils/FileCache.hpp"
#include "utils/Logger.hpp"
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <algorithm>
#include <filesystem>
#include <vector>

namespace FileUtils {

static FileCache fileCache(100);

static bool validateSecurity(std::string_view uri, HTTP::StatusCode& status) {
    if (!isPathSafe(uri)) {
        Logger::warnf("Security violation: unsafe path detected");
        status = HTTP::StatusCode::FORBIDDEN;
        return false;
    }
    return true;
}

static std::optional<struct stat> getFileStats(const std::string& filePath) {
    struct stat fileStat;
    return (stat(filePath.c_str(), &fileStat) == 0) ? std::make_optional(fileStat) : std::nullopt;
}

static std::optional<std::string> findIndexFile(std::string_view uri, const std::string& dirPath) {
    const char* indexFiles[] = {"index.html", "index.htm", "index.php"};
    
    for (const char* indexFile : indexFiles) {
        std::string indexPath = dirPath + "/" + indexFile;
        if (access(indexPath.c_str(), R_OK) == 0) {
            return std::string(uri) + (uri.back() == '/' ? "" : "/") + indexFile;
        }
    }
    return std::nullopt;
}

std::string readFile(std::string_view rootDir, std::string_view uri, HTTP::StatusCode &status) {
    const std::string filePath = buildPath(rootDir, uri);
    
    if (!validateSecurity(uri, status)) return "";

    std::string content, mimeType;
    if (fileCache.getFile(filePath, content, mimeType)) {
        status = HTTP::StatusCode::OK;
        return content;
    }

    auto fileStat = getFileStats(filePath);
    if (!fileStat) {
        status = HTTP::StatusCode::NOT_FOUND;
        return "";
    }

    if (S_ISDIR(fileStat->st_mode)) {
        auto indexUri = findIndexFile(uri, filePath);
        if (indexUri) {
            return readFile(rootDir, *indexUri, status);
        }
        status = HTTP::StatusCode::FORBIDDEN;
        return "";
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open() || access(filePath.c_str(), R_OK) != 0) {
        status = HTTP::StatusCode::FORBIDDEN;
        return "";
    }
    
    content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    fileCache.cacheFile(filePath, content, "text/html");
    status = HTTP::StatusCode::OK;
    return content;
}

bool writeFile(std::string_view rootDir, std::string_view uri, std::string_view content, HTTP::StatusCode &status) {
    const std::string filePath = buildPath(rootDir, uri);
    
    if (!validateSecurity(uri, status)) return false;
    
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        status = HTTP::StatusCode::INTERNAL_SERVER_ERROR;
        return false;
    }
    
    file << content;
    if (file.good()) {
        fileCache.cacheFile(filePath, std::string(content), "text/html");
        status = HTTP::StatusCode::CREATED;
        return true;
    }
    
    status = HTTP::StatusCode::INTERNAL_SERVER_ERROR;
    return false;
}

bool deleteFile(std::string_view rootDir, std::string_view uri, HTTP::StatusCode &status) {
    const std::string filePath = buildPath(rootDir, uri);
    
    if (!validateSecurity(uri, status)) return false;
    
    auto fileStat = getFileStats(filePath);
    if (!fileStat) {
        status = HTTP::StatusCode::NOT_FOUND;
        return false;
    }
    
    if (S_ISDIR(fileStat->st_mode)) {
        status = HTTP::StatusCode::FORBIDDEN;
        return false;
    }
    
    if (unlink(filePath.c_str()) == 0) {
        status = HTTP::StatusCode::NO_CONTENT;
        return true;
    }
    
    status = HTTP::StatusCode::INTERNAL_SERVER_ERROR;
    return false;
}

std::optional<size_t> fileExists(std::string_view rootDir, std::string_view uri) {
    const auto fileStat = getFileStats(buildPath(rootDir, uri));
    return fileStat ? std::make_optional(fileStat->st_size) : std::nullopt;
}

std::string buildPath(std::string_view root, std::string_view path) {
    const std::string cleanPath = sanitizePath(path);
    return (cleanPath.empty() || cleanPath == "/") 
        ? std::string(root) + "/index.html"
        : std::string(root) + cleanPath;
}

bool isPathSafe(std::string_view path) {
    return path.find("..") == std::string_view::npos;
}

std::filesystem::path canonicalizePath(std::string_view path) {
    try {
        return std::filesystem::canonical(std::filesystem::path(path));
    } catch (const std::filesystem::filesystem_error&) {
        return std::filesystem::path{};
    }
}

std::string sanitizePath(std::string_view path) {
    if (path.empty()) return "/";
    
    std::string result{path};
    if (result[0] != '/') result = "/" + result;
    
    for (size_t pos = 0; (pos = result.find("//", pos)) != std::string::npos;) {
        result.replace(pos, 2, "/");
    }
    
    return result;
}

std::optional<std::string> readFileContent(std::string_view filePath) {
    std::ifstream file(std::string(filePath), std::ios::binary);
    return file.is_open() 
        ? std::make_optional(std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()))
        : std::nullopt;
}

bool writeFileContent(std::string_view filePath, std::string_view content) {
    std::ofstream file(std::string(filePath), std::ios::binary);
    return file.is_open() && (file << content) && file.good();
}

void clearCache() {
    fileCache.clearCache();
}

void setCacheMaxSize(size_t maxSize) {
    fileCache = FileCache(maxSize);
}

bool isDirectory(std::string_view path) {
    struct stat pathStat;
    return stat(std::string(path).c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode);
}

std::string generateDirectoryListing(std::string_view dirPath, std::string_view uri) {
    std::string html = 
        "<html><head><title>Directory listing for " + std::string(uri) + "</title>"
        "<style>body{font-family:Arial,sans-serif;margin:20px}h1{color:#333;border-bottom:1px solid #ccc}"
        "ul{list-style-type:none;padding:0}li{margin:5px 0}a{text-decoration:none;color:#0066cc}"
        "a:hover{text-decoration:underline}.dir{font-weight:bold}.file{color:#666}</style></head><body>"
        "<h1>Directory listing for " + std::string(uri) + "</h1><hr><ul>";
    
    if (uri != "/") {
        html += "<li><a href=\"../\" class=\"dir\">../</a></li>";
    }
    
    DIR* dir = opendir(std::string(dirPath).c_str());
    if (!dir) {
        html += "</ul><hr><em>Error reading directory</em></body></html>";
        return html;
    }
    
    std::vector<std::pair<std::string, bool>> entries;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        
        std::string fullPath = std::string(dirPath) + "/" + name;
        entries.emplace_back(name, isDirectory(fullPath));
    }
    closedir(dir);
    
    std::sort(entries.begin(), entries.end(), 
              [](const auto& a, const auto& b) {
                  return a.second != b.second ? a.second > b.second : a.first < b.first;
              });
    
    for (const auto& [name, isDir] : entries) {
        const std::string href = name + (isDir ? "/" : "");
        const std::string cssClass = isDir ? "dir" : "file";
        const std::string displayName = name + (isDir ? "/" : "");
        html += "<li><a href=\"" + href + "\" class=\"" + cssClass + "\">" + displayName + "</a></li>";
    }
    
    html += "</ul><hr><em>Generated by WebServ</em></body></html>";
    return html;
}

std::string extractQueryParams(std::string_view uri) {
    const size_t queryPos = uri.find('?');
    return (queryPos != std::string_view::npos) ? std::string(uri.substr(queryPos + 1)) : "";
}

std::string cleanUri(std::string_view uri) {
    const size_t queryPos = uri.find('?');
    return (queryPos != std::string_view::npos) ? std::string(uri.substr(0, queryPos)) : std::string(uri);
}

}