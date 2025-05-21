#pragma once

#include <string>
#include <unordered_map>

class FileType {
private:
    static std::unordered_map<std::string, std::string> _mimeTypes;
    static std::unordered_map<std::string, std::string> _cgiHandlers;

public:
    // Get content type based on file extension
    static std::string getMimeType(const std::string& path);
    
    // Check if file extension is a CGI script
    static bool isCGI(const std::string& path);
    
    // Get CGI handler for a given extension
    static std::string getCGIHandler(const std::string& path);
    
    // Helper to extract extension from a path
    static std::string getExtension(const std::string& path);
    
    // Register a new MIME type
    static void registerMimeType(const std::string& extension, const std::string& mimeType);
    
    // Register a new CGI handler
    static void registerCGIHandler(const std::string& extension, const std::string& handler);
};