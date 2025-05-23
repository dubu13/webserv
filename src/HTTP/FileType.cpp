#include "HTTP/FileType.hpp"
#include <algorithm>

// Initialize static members with common MIME types
std::unordered_map<std::string, std::string> FileType::_mimeTypes = {
    {"html", "text/html"},
    {"htm", "text/html"},
    {"css", "text/css"},
    {"js", "application/javascript"},
    {"json", "application/json"},
    {"png", "image/png"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"gif", "image/gif"},
    {"svg", "image/svg+xml"},
    {"ico", "image/x-icon"},
    {"txt", "text/plain"},
    {"pdf", "application/pdf"},
    {"xml", "application/xml"}
};

// Initialize CGI handlers
std::unordered_map<std::string, std::string> FileType::_cgiHandlers = {
    {"php", "/usr/bin/php-cgi"},
    {"py", "/usr/bin/python3"},
    {"pl", "/usr/bin/perl"},
    {"rb", "/usr/bin/ruby"},
    {"sh", "/usr/bin/bash"},
    {"js", "/home/dkremer/.nvm/versions/node/v22.14.0/bin/node"}
};

std::string FileType::getMimeType(const std::string& path) {
    std::string ext = getExtension(path);
    if (ext.empty() || _mimeTypes.find(ext) == _mimeTypes.end())
        return "application/octet-stream";  // Default binary type
    return _mimeTypes[ext];
}

bool FileType::isCGI(const std::string& path) {
    std::string ext = getExtension(path);
    return !ext.empty() && _cgiHandlers.find(ext) != _cgiHandlers.end();
}

std::string FileType::getCGIHandler(const std::string& path) {
    std::string ext = getExtension(path);
    if (ext.empty() || _cgiHandlers.find(ext) == _cgiHandlers.end())
        return "";
    return _cgiHandlers[ext];
}

std::string FileType::getExtension(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos || dotPos == path.length() - 1)
        return "";
    return path.substr(dotPos + 1);
}

void FileType::registerMimeType(const std::string& extension, const std::string& mimeType) {
    _mimeTypes[extension] = mimeType;
}

void FileType::registerCGIHandler(const std::string& extension, const std::string& handler) {
    _cgiHandlers[extension] = handler;
}