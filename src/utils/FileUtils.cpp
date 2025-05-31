#include "utils/FileUtils.hpp"
#include "utils/FileCache.hpp"
#include "utils/Logger.hpp"
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <dirent.h>
#include <algorithm>

namespace FileUtils {

// Static file cache instance
static FileCache fileCache(100); // Cache up to 100 files

std::string readFile(const std::string &rootDir, const std::string &uri, HTTP::StatusCode &status) {
    std::string filePath = buildPath(rootDir, uri);
    Logger::debugf("FileUtils::readFile - rootDir: %s, uri: %s, built path: %s", 
                   rootDir.c_str(), uri.c_str(), filePath.c_str());
    
    if (!isPathSafe(uri)) {
        Logger::warnf("Security violation: unsafe path detected: %s", uri.c_str());
        status = HTTP::StatusCode::FORBIDDEN;
        return "";
    }
    
    // Try to get from cache first
    std::string content, mimeType;
    if (fileCache.getFile(filePath, content, mimeType)) {
        Logger::debugf("Cache HIT: file found in cache: %s (%zu bytes)", filePath.c_str(), content.size());
        status = HTTP::StatusCode::OK;
        return content;
    }
    
    Logger::debugf("Cache MISS: attempting to read file from disk: %s", filePath.c_str());
    
    // Check if file exists and get stats
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) != 0) {
        Logger::warnf("File not found: %s (errno: %d - %s)", filePath.c_str(), errno, strerror(errno));
        status = HTTP::StatusCode::NOT_FOUND;
        return "";
    }
    
    // Check if it's a directory
    if (S_ISDIR(fileStat.st_mode)) {
        Logger::debugf("Path is a directory, checking for index files: %s", filePath.c_str());
        // Try to find index files
        const char* indexFileArray[] = {"index.html", "index.htm", "index.php"};
        for (size_t i = 0; i < sizeof(indexFileArray) / sizeof(indexFileArray[0]); ++i) {
            std::string indexPath = filePath;
            if (indexPath.back() != '/') indexPath += "/";
            indexPath += indexFileArray[i];
            
            Logger::debugf("Checking for index file: %s", indexPath.c_str());
            if (access(indexPath.c_str(), R_OK) == 0) {
                Logger::infof("Found index file: %s", indexPath.c_str());
                return readFile(rootDir, uri + (uri.back() == '/' ? "" : "/") + indexFileArray[i], status);
            }
        }
        Logger::warnf("Directory access without index file: %s", filePath.c_str());
        status = HTTP::StatusCode::FORBIDDEN;
        return "";
    }
    
    // Check if file is readable
    if (access(filePath.c_str(), R_OK) != 0) {
        Logger::warnf("File not readable: %s (errno: %d - %s)", filePath.c_str(), errno, strerror(errno));
        status = HTTP::StatusCode::FORBIDDEN;
        return "";
    }
    
    // If not in cache, read from file
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        Logger::errorf("Failed to open file for reading: %s", filePath.c_str());
        status = HTTP::StatusCode::INTERNAL_SERVER_ERROR;
        return "";
    }
    
    content = std::string((std::istreambuf_iterator<char>(file)), 
                         std::istreambuf_iterator<char>());
    file.close();
    
    Logger::infof("Successfully read %zu bytes from file: %s", content.size(), filePath.c_str());
    
    // Cache the file content
    fileCache.cacheFile(filePath, content, "text/html"); // Default MIME type
    
    status = HTTP::StatusCode::OK;
    return content;
}

bool writeFile(const std::string &rootDir, const std::string &uri, 
               const std::string &content, HTTP::StatusCode &status) {
    std::string filePath = buildPath(rootDir, uri);
    Logger::debugf("FileUtils::writeFile - attempting to write %zu bytes to: %s", content.size(), filePath.c_str());
    
    if (!isPathSafe(uri)) {
        Logger::warnf("Security violation: unsafe path detected for write: %s", uri.c_str());
        status = HTTP::StatusCode::FORBIDDEN;
        return false;
    }
    
    // Check if directory exists and create if necessary
    size_t lastSlash = filePath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        std::string dirPath = filePath.substr(0, lastSlash);
        Logger::debugf("Checking directory: %s", dirPath.c_str());
        
        struct stat dirStat;
        if (stat(dirPath.c_str(), &dirStat) != 0) {
            Logger::debugf("Directory doesn't exist, attempting to create: %s", dirPath.c_str());
            // You might want to implement recursive directory creation here
        }
    }
    
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        Logger::errorf("Failed to open file for writing: %s (errno: %d - %s)", 
                      filePath.c_str(), errno, strerror(errno));
        status = HTTP::StatusCode::INTERNAL_SERVER_ERROR;
        return false;
    }
    
    file << content;
    bool success = file.good();
    file.close();
    
    if (success) {
        Logger::infof("Successfully wrote %zu bytes to file: %s", content.size(), filePath.c_str());
        // Cache the newly written file
        fileCache.cacheFile(filePath, content, "text/html"); // Default MIME type
        status = HTTP::StatusCode::CREATED;
        return true;
    } else {
        Logger::errorf("Failed to write content to file: %s", filePath.c_str());
        status = HTTP::StatusCode::INTERNAL_SERVER_ERROR;
        return false;
    }
}

bool deleteFile(const std::string &rootDir, const std::string &uri, HTTP::StatusCode &status) {
    std::string filePath = buildPath(rootDir, uri);
    Logger::debugf("FileUtils::deleteFile - attempting to delete: %s", filePath.c_str());
    
    if (!isPathSafe(uri)) {
        Logger::warnf("Security violation: unsafe path detected for delete: %s", uri.c_str());
        status = HTTP::StatusCode::FORBIDDEN;
        return false;
    }
    
    // Check if file exists first
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) != 0) {
        Logger::warnf("File not found for deletion: %s (errno: %d - %s)", 
                     filePath.c_str(), errno, strerror(errno));
        status = HTTP::StatusCode::NOT_FOUND;
        return false;
    }
    
    // Check if it's a directory
    if (S_ISDIR(fileStat.st_mode)) {
        Logger::warnf("Attempted to delete directory: %s", filePath.c_str());
        status = HTTP::StatusCode::FORBIDDEN;
        return false;
    }
    
    if (unlink(filePath.c_str()) == 0) {
        Logger::infof("Successfully deleted file: %s", filePath.c_str());
        // Note: FileCache doesn't support individual file removal, so cache will be stale
        // This is acceptable for a basic implementation
        status = HTTP::StatusCode::NO_CONTENT;
        return true;
    } else {
        Logger::errorf("Failed to delete file: %s (errno: %d - %s)", 
                      filePath.c_str(), errno, strerror(errno));
        status = HTTP::StatusCode::INTERNAL_SERVER_ERROR;
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

// Directory utilities
bool isDirectory(const std::string &path) {
    struct stat pathStat;
    return stat(path.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode);
}

std::string generateDirectoryListing(const std::string &dirPath, const std::string &uri) {
    Logger::debugf("Generating directory listing for: %s (URI: %s)", dirPath.c_str(), uri.c_str());
    
    std::string html = "<html><head><title>Directory listing for " + uri + "</title>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; }";
    html += "h1 { color: #333; border-bottom: 1px solid #ccc; }";
    html += "ul { list-style-type: none; padding: 0; }";
    html += "li { margin: 5px 0; }";
    html += "a { text-decoration: none; color: #0066cc; }";
    html += "a:hover { text-decoration: underline; }";
    html += ".dir { font-weight: bold; }";
    html += ".file { color: #666; }";
    html += "</style>";
    html += "</head><body>";
    html += "<h1>Directory listing for " + uri + "</h1><hr><ul>";
    
    // Add parent directory link if not root
    if (uri != "/") {
        html += "<li><a href=\"../\" class=\"dir\">../</a></li>";
    }
    
    // Open directory and read entries
    DIR* dir = opendir(dirPath.c_str());
    if (dir == nullptr) {
        Logger::errorf("Failed to open directory: %s (errno: %d - %s)", 
                      dirPath.c_str(), errno, strerror(errno));
        html += "</ul><hr><em>Error reading directory</em></body></html>";
        return html;
    }
    
    // Read directory entries
    std::vector<std::pair<std::string, bool>> entries; // name, isDirectory
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        
        // Skip . and .. entries (we handle .. separately above)
        if (name == "." || name == "..") {
            continue;
        }
        
        // Check if it's a directory
        std::string fullPath = dirPath + "/" + name;
        bool isDir = isDirectory(fullPath);
        entries.push_back({name, isDir});
    }
    closedir(dir);
    
    // Sort entries: directories first, then files, alphabetically
    std::sort(entries.begin(), entries.end(), 
              [](const std::pair<std::string, bool>& a, const std::pair<std::string, bool>& b) {
                  if (a.second != b.second) return a.second > b.second; // directories first
                  return a.first < b.first; // alphabetical
              });
    
    // Generate HTML for entries
    for (const auto& entry : entries) {
        const std::string& name = entry.first;
        bool isDir = entry.second;
        
        std::string href = name;
        if (isDir) {
            href += "/";
        }
        
        std::string cssClass = isDir ? "dir" : "file";
        std::string displayName = name + (isDir ? "/" : "");
        
        html += "<li><a href=\"" + href + "\" class=\"" + cssClass + "\">" + displayName + "</a></li>";
    }
    
    html += "</ul><hr><em>Generated by WebServ</em></body></html>";
    
    Logger::debugf("Generated directory listing with %zu entries", entries.size());
    return html;
}

// Path utilities
std::string extractQueryParams(const std::string &uri) {
    size_t queryPos = uri.find('?');
    return (queryPos != std::string::npos) ? uri.substr(queryPos + 1) : "";
}

std::string cleanUri(const std::string &uri) {
    size_t queryPos = uri.find('?');
    return (queryPos != std::string::npos) ? uri.substr(0, queryPos) : uri;
}

}