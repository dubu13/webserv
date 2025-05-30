#include "utils/FileUtils.hpp"
#include "utils/FileCache.hpp"
#include "utils/Logger.hpp"
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

namespace FileUtils {

// Static file cache instance
static FileCache fileCache(100); // Cache up to 100 files

std::string readFile(const std::string &rootDir, const std::string &uri, HTTP::StatusCode &status) {
    std::string filePath = buildPath(rootDir, uri);
    Logger::debugf("FileUtils::readFile - rootDir: %s, uri: %s, built path: %s", 
                   rootDir.c_str(), uri.c_str(), filePath.c_str());
    
    if (!isPathSafe(uri)) {
        Logger::warnf("Unsafe path detected: %s", uri.c_str());
        status = HTTP::StatusCode::FORBIDDEN;
        return "";
    }
    
    // Try to get from cache first
    std::string content, mimeType;
    if (fileCache.getFile(filePath, content, mimeType)) {
        Logger::debugf("File found in cache: %s", filePath.c_str());
        status = HTTP::StatusCode::OK;
        return content;
    }
    
    // If not in cache, read from file
    Logger::debugf("Attempting to open file: %s", filePath.c_str());
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        Logger::warnf("Failed to open file: %s", filePath.c_str());
        status = HTTP::StatusCode::NOT_FOUND;
        return "";
    }
    
    content = std::string((std::istreambuf_iterator<char>(file)), 
                         std::istreambuf_iterator<char>());
    
    Logger::debugf("Successfully read %zu bytes from file: %s", content.size(), filePath.c_str());
    
    // Cache the file content
    fileCache.cacheFile(filePath, content, "text/html"); // Default MIME type
    
    status = HTTP::StatusCode::OK;
    return content;
}

bool writeFile(const std::string &rootDir, const std::string &uri, 
               const std::string &content, HTTP::StatusCode &status) {
    std::string filePath = buildPath(rootDir, uri);
    
    if (!isPathSafe(uri)) {
        status = HTTP::StatusCode::FORBIDDEN;
        return false;
    }
    
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        status = HTTP::StatusCode::INTERNAL_SERVER_ERROR;
        return false;
    }
    
    file << content;
    if (file.good()) {
        // Cache the newly written file
        fileCache.cacheFile(filePath, content, "text/html"); // Default MIME type
        status = HTTP::StatusCode::CREATED;
        return true;
    } else {
        status = HTTP::StatusCode::INTERNAL_SERVER_ERROR;
        return false;
    }
}

bool deleteFile(const std::string &rootDir, const std::string &uri, HTTP::StatusCode &status) {
    std::string filePath = buildPath(rootDir, uri);
    
    if (!isPathSafe(uri)) {
        status = HTTP::StatusCode::FORBIDDEN;
        return false;
    }
    
    if (unlink(filePath.c_str()) == 0) {
        status = HTTP::StatusCode::NO_CONTENT;
        return true;
    } else {
        status = HTTP::StatusCode::NOT_FOUND;
        return false;
    }
}

bool fileExists(const std::string &rootDir, const std::string &uri, size_t &size) {
    std::string filePath = buildPath(rootDir, uri);
    struct stat st;
    
    if (stat(filePath.c_str(), &st) == 0) {
        size = st.st_size;
        return true;
    }
    return false;
}

std::string buildPath(const std::string &root, const std::string &path) {
    std::string cleanPath = sanitizePath(path);
    if (cleanPath.empty() || cleanPath == "/") {
        return root + "/index.html";
    }
    return root + cleanPath;
}

bool isPathSafe(const std::string &path) {
    // Prevent directory traversal attacks
    return path.find("..") == std::string::npos;
}

std::string sanitizePath(const std::string &path) {
    if (path.empty()) return "/";
    
    std::string result = path;
    if (result[0] != '/') {
        result = "/" + result;
    }
    
    // Remove double slashes
    size_t pos = 0;
    while ((pos = result.find("//", pos)) != std::string::npos) {
        result.replace(pos, 2, "/");
    }
    
    return result;
}

std::string readFileContent(const std::string &filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    return std::string((std::istreambuf_iterator<char>(file)), 
                       std::istreambuf_iterator<char>());
}

bool writeFileContent(const std::string &filePath, const std::string &content) {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file << content;
    return file.good();
}

void clearCache() {
    fileCache.clearCache();
}

void setCacheMaxSize(size_t maxSize) {
    // Create a new cache with the specified size
    fileCache = FileCache(maxSize);
}

}