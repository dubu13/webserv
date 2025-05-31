#pragma once
#include "HTTP/HTTPTypes.hpp"
#include "HTTP/HttpParser.hpp"
#include "config/ServerBlock.ipp"
#include "HTTP/HttpResponseBuilder.hpp"
#include "utils/FileUtils.hpp"
#include "utils/Logger.hpp"
#include <map>
#include <string>
#include <functional>
#include <ctime>

// HTTP Request/Response handler implementations
// These are inline implementations to be included in HTTPHandler.cpp

namespace HTTP {

// Request validation utilities (SOLID: Single Responsibility)
namespace RequestValidator {
    inline bool isMethodAllowed(const HttpParser::Request& request, const LocationBlock* location) {
        if (!location || location->allowedMethods.empty()) {
            return true; // No restrictions
        }
        
        std::string methodStr = HTTP::methodToString(request.requestLine.method);
        bool allowed = location->allowedMethods.find(methodStr) != location->allowedMethods.end();
        if (!allowed) {
            Logger::warnf("Method %s not allowed for URI %s", methodStr.c_str(), request.requestLine.uri.c_str());
        }
        return allowed;
    }
    
    inline bool hasRequiredAuth(const HttpParser::Request& request) {
        // Simple check for Authorization header (can be extended)
        return request.headers.find("Authorization") != request.headers.end();
    }
    
    inline std::string getHeaderValue(const HttpParser::Request& request, const std::string& headerName) {
        auto it = request.headers.find(headerName);
        return (it != request.headers.end()) ? it->second : "";
    }
}

// Path resolution utilities (SOLID: Single Responsibility)
namespace PathResolver {
    inline std::string resolveIndexFile(const std::string& dirPath, const LocationBlock* location) {
        if (location && !location->index.empty()) {
            std::string indexPath = dirPath + "/" + location->index;
            size_t fileSize;
            if (FileUtils::fileExists(dirPath, "/" + location->index, fileSize)) {
                return indexPath;
            }
        }
        return "";
    }
    
    inline std::string cleanRequestUri(const std::string& uri) {
        return FileUtils::cleanUri(uri);
    }
    
    // Directory handling for GET requests
    inline std::string handleDirectoryRequest(const std::string& filePath, 
                                            const std::string& cleanUri, 
                                            const LocationBlock* location,
                                            std::string& updatedFilePath,
                                            std::string& updatedCleanUri) {
        Logger::debugf("Handling directory request for: %s", filePath.c_str());
        
        if (location) {
            if (!location->index.empty()) {
                std::string indexFile = PathResolver::resolveIndexFile(filePath, location);
                if (!indexFile.empty()) {
                    Logger::debugf("Directory access, using index file: %s", indexFile.c_str());
                    // Update the references for further processing
                    updatedFilePath = indexFile;
                    updatedCleanUri = cleanUri + "/" + location->index;
                    return ""; // Empty string indicates to continue with file serving
                }
            }
            
            if (location->autoindex) {
                Logger::debug("Directory autoindex enabled");
                std::string listing = FileUtils::generateDirectoryListing(filePath, cleanUri);
                return HttpResponseBuilder::createFileResponse(HTTP::StatusCode::OK, listing, "text/html");
            }
        }
        
        Logger::warn("Directory access forbidden (no location configuration or no index/autoindex)");
        return HttpResponseBuilder::createErrorResponse(HTTP::StatusCode::FORBIDDEN);
    }
}

// Forward declarations - Remove this since we need the full definition
// struct Request;

// Handler function type for different HTTP methods
using MethodHandler = std::function<std::string(const HttpParser::Request&, const std::string&, const std::string&, const LocationBlock*)>;

// HTTP Method Handler implementations
inline std::string handleGetRequest(const HttpParser::Request& request, 
                                   const std::string& effectiveRoot, 
                                   const std::string& effectiveUri, 
                                   const LocationBlock* location) {
    (void)request; // Suppress unused parameter warning - may be used for future enhancements
    Logger::debug("Processing GET request");
    
    // Clean URI from query parameters
    std::string cleanUri = PathResolver::cleanRequestUri(effectiveUri);
    std::string filePath = effectiveRoot + cleanUri;
    
    Logger::debugf("Final file path: %s (root: %s, clean_uri: %s)", filePath.c_str(), effectiveRoot.c_str(), cleanUri.c_str());
    
    // Check if path is directory and handle index files
    if (FileUtils::isDirectory(filePath)) {
        std::string updatedFilePath = filePath;
        std::string updatedCleanUri = cleanUri;
        std::string directoryResponse = PathResolver::handleDirectoryRequest(filePath, cleanUri, location, updatedFilePath, updatedCleanUri);
        
        // If we got a complete response (error or directory listing), return it
        if (!directoryResponse.empty()) {
            return directoryResponse;
        }
        
        // If we got updated paths for index file, use them
        filePath = updatedFilePath;
        cleanUri = updatedCleanUri;
        Logger::debugf("Using resolved index file: %s", filePath.c_str());
    }
    
    // Handle static file serving
    HTTP::StatusCode status;
    std::string content = FileUtils::readFile(effectiveRoot, cleanUri, status);
    Logger::debugf("File read result: status=%d, content_length=%zu", static_cast<int>(status), content.length());
    
    if (status != HTTP::StatusCode::OK) {
        Logger::warnf("File access failed with status %d for path: %s", static_cast<int>(status), filePath.c_str());
        return HttpResponseBuilder::createErrorResponse(status);
    }
    
    std::string mimeType = HTTP::getMimeType(filePath);
    Logger::debugf("Serving file with MIME type: %s", mimeType.c_str());
    return HttpResponseBuilder::createFileResponse(status, content, mimeType);
}

inline std::string handlePostRequest(const HttpParser::Request& request, const std::string& effectiveRoot, 
                                    const std::string& effectiveUri, const LocationBlock* location) {
    Logger::debug("Processing POST request");
    
    // Validate method is allowed
    if (!RequestValidator::isMethodAllowed(request, location)) {
        return HttpResponseBuilder::createErrorResponse(HTTP::StatusCode::METHOD_NOT_ALLOWED);
    }
    
    // Check upload is enabled for this location
    if (location && !location->uploadEnable) {
        Logger::warn("Upload not enabled for this location");
        return HttpResponseBuilder::createErrorResponse(HTTP::StatusCode::FORBIDDEN);
    }
    
    // Get and validate Content-Type
    auto contentType = RequestValidator::getHeaderValue(request, "Content-Type");
    Logger::debugf("POST Content-Type: %s", contentType.c_str());
    
    // Handle file upload
    HTTP::StatusCode status;
    bool success = false;

    if (location && !location->uploadStore.empty()) {
        // Use configured upload directory
        std::string uploadDir = location->uploadStore;
        std::string filename = "upload_" + std::to_string(time(nullptr)) + ".txt";
        std::string uploadPath = "/" + filename;
        Logger::debugf("Uploading to configured directory: %s%s", uploadDir.c_str(), uploadPath.c_str());
        success = FileUtils::writeFile(uploadDir, uploadPath, request.body, status);
    } else {
        // Fallback to default behavior
        Logger::debugf("Uploading to default location: %s", effectiveUri.c_str());
        success = FileUtils::writeFile(effectiveRoot, effectiveUri, request.body, status);
    }

    Logger::debugf("Upload result: success=%s, status=%d", success ? "true" : "false", static_cast<int>(status));
    return HttpResponseBuilder::createSimpleResponse(
        status, success ? "File uploaded successfully" : "Upload failed");
}

inline std::string handleDeleteRequest(const HttpParser::Request& request, const std::string& effectiveRoot, 
                                      const std::string& effectiveUri, const LocationBlock* location) {
    Logger::debugf("Processing DELETE request for: %s", effectiveUri.c_str());
    
    // Check if DELETE is allowed in this location (SOLID: Open/Closed Principle)
    if (!RequestValidator::isMethodAllowed(request, location)) {
        return HttpResponseBuilder::createErrorResponse(HTTP::StatusCode::METHOD_NOT_ALLOWED);
    }
    
    // Optional: Log request headers for debugging
    auto contentType = RequestValidator::getHeaderValue(request, "Content-Type");
    if (!contentType.empty()) {
        Logger::debugf("DELETE request Content-Type: %s", contentType.c_str());
    }
    
    HTTP::StatusCode status;
    bool success = FileUtils::deleteFile(effectiveRoot, effectiveUri, status);
    Logger::debugf("Delete result: success=%s, status=%d", success ? "true" : "false", static_cast<int>(status));
    
    return HttpResponseBuilder::createSimpleResponse(status, success ? "File deleted" : "Delete failed");
}

// Request processing utilities
inline std::pair<std::string, std::string> resolveEffectivePath(const std::string& uri, 
                                                               const std::string& rootDirectory, 
                                                               const LocationBlock* location) {
    std::string effectiveRoot = rootDirectory;
    std::string effectiveUri = uri;
    
    if (location && !location->root.empty()) {
        Logger::debugf("Found location block for URI %s with root: %s", uri.c_str(), location->root.c_str());
        effectiveRoot = location->root;
        
        Logger::debugf("Using location-specific root: %s for URI: %s", effectiveRoot.c_str(), uri.c_str());
    } else {
        Logger::debugf("No specific location found for URI %s, using default root", uri.c_str());
    }
    
    return {effectiveRoot, effectiveUri};
}

// Error handling utilities
inline std::string handleErrorWithCustomPage(HTTP::StatusCode status, 
                                            const std::map<HTTP::StatusCode, std::string>& customErrorPages,
                                            const std::string& rootDirectory) {
    auto errorPageIt = customErrorPages.find(status);
    if (errorPageIt != customErrorPages.end()) {
        Logger::debugf("Using custom error page: %s", errorPageIt->second.c_str());
        HTTP::StatusCode fileStatus;
        std::string customContent = FileUtils::readFile(rootDirectory, errorPageIt->second, fileStatus);
        if (fileStatus == HTTP::StatusCode::OK) {
            Logger::debug("Custom error page loaded successfully");
            return HttpResponseBuilder::createFileResponse(status, customContent, "text/html");
        } else {
            Logger::debug("Custom error page failed to load, using default");
        }
    }
    return HttpResponseBuilder::createErrorResponse(status);
}

// Method dispatcher using hash map for O(1) lookup
inline const std::map<HTTP::Method, MethodHandler>& getMethodHandlers() {
    static const std::map<HTTP::Method, MethodHandler> handlers = {
        {HTTP::Method::GET, handleGetRequest},
        {HTTP::Method::POST, handlePostRequest},
        {HTTP::Method::DELETE, handleDeleteRequest}
        // Add more methods as needed: HEAD, PUT, PATCH, etc.
    };
    return handlers;
}

inline std::string dispatchMethodHandler(const HttpParser::Request& request, 
                                        const std::string& effectiveRoot,
                                        const std::string& effectiveUri, 
                                        const LocationBlock* location) {
    const auto& handlers = getMethodHandlers();
    auto it = handlers.find(request.requestLine.method);
    
    if (it != handlers.end()) {
        Logger::debugf("Dispatching to handler for method: %s", HTTP::methodToString(request.requestLine.method).c_str());
        return it->second(request, effectiveRoot, effectiveUri, location);
    } else {
        Logger::warnf("Method not implemented: %s", HTTP::methodToString(request.requestLine.method).c_str());
        return HttpResponseBuilder::createErrorResponse(HTTP::StatusCode::METHOD_NOT_ALLOWED);
    }
}

} // namespace HTTP
