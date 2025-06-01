#include "HTTP/HTTPHandlers.hpp"
#include "config/LocationBlock.ipp"
#include <ctime>

namespace HTTP {

namespace RequestValidator {
    bool isMethodAllowed(const HTTP::Request& request, const LocationBlock* location) {
        if (!location || location->allowedMethods.empty()) {
            return true;
        }
        
        std::string methodStr = HTTP::methodToString(request.requestLine.method);
        return location->allowedMethods.find(methodStr) != location->allowedMethods.end();
    }
    
    bool hasRequiredAuth(const HTTP::Request& request) {
        return request.headers.find("Authorization") != request.headers.end();
    }
    
    std::string getHeaderValue(const HTTP::Request& request, const std::string& headerName) {
        auto it = request.headers.find(headerName);
        return (it != request.headers.end()) ? it->second : "";
    }
}

namespace PathResolver {
    std::string resolveIndexFile(const std::string& dirPath, const LocationBlock* location) {
        if (location && !location->index.empty()) {
            std::string indexPath = dirPath + "/" + location->index;
            auto fileSize = FileUtils::fileExists(dirPath, "/" + location->index);
            if (fileSize.has_value()) {
                return indexPath;
            }
        }
        return "";
    }
    
    std::string cleanRequestUri(const std::string& uri) {
        return FileUtils::cleanUri(uri);
    }
    
    std::string handleDirectoryRequest(const std::string& filePath, 
                                     const std::string& cleanUri, 
                                     const LocationBlock* location,
                                     std::string& updatedFilePath,
                                     std::string& updatedCleanUri) {
        if (location) {
            if (!location->index.empty()) {
                std::string indexFile = PathResolver::resolveIndexFile(filePath, location);
                if (!indexFile.empty()) {
                    updatedFilePath = indexFile;
                    updatedCleanUri = cleanUri + "/" + location->index;
                    return "";
                }
            }
            
            if (location->autoindex) {
                std::string listing = FileUtils::generateDirectoryListing(filePath, cleanUri);
                return HTTP::ResponseBuilder::createFileResponse(HTTP::StatusCode::OK, listing, "text/html");
            }
        }
        
        return HTTP::ResponseBuilder::createErrorResponse(HTTP::StatusCode::FORBIDDEN);
    }
}

std::string handleGetRequest(const HTTP::Request& request, 
                           const std::string& effectiveRoot, 
                           const std::string& effectiveUri, 
                           const LocationBlock* location) {
    (void)request;
    
    std::string cleanUri = PathResolver::cleanRequestUri(effectiveUri);
    std::string filePath = effectiveRoot + cleanUri;
    
    if (FileUtils::isDirectory(filePath)) {
        std::string updatedFilePath = filePath;
        std::string updatedCleanUri = cleanUri;
        std::string directoryResponse = PathResolver::handleDirectoryRequest(filePath, cleanUri, location, updatedFilePath, updatedCleanUri);
        
        if (!directoryResponse.empty()) {
            return directoryResponse;
        }
        
        filePath = updatedFilePath;
        cleanUri = updatedCleanUri;
    }
    
    HTTP::StatusCode status;
    std::string content = FileUtils::readFile(effectiveRoot, cleanUri, status);
    
    if (status != HTTP::StatusCode::OK) {
        return HTTP::ResponseBuilder::createErrorResponse(status);
    }
    
    std::string mimeType = HTTP::getMimeType(filePath);
    
    if (content.length() > CHUNKED_THRESHOLD) {
        return HTTP::ResponseBuilder::createChunkedFileResponse(status, content, mimeType);
    } else {
        return HTTP::ResponseBuilder::createFileResponse(status, content, mimeType);
    }
}

std::string handlePostRequest(const HTTP::Request& request, const std::string& effectiveRoot, 
                            const std::string& effectiveUri, const LocationBlock* location) {
    if (!RequestValidator::isMethodAllowed(request, location)) {
        return HTTP::ResponseBuilder::createErrorResponse(HTTP::StatusCode::METHOD_NOT_ALLOWED);
    }
    if (location && !location->uploadEnable) {
        return HTTP::ResponseBuilder::createErrorResponse(HTTP::StatusCode::FORBIDDEN);
    }
    
    // Get and validate Content-Type
    auto contentType = RequestValidator::getHeaderValue(request, "Content-Type");
    
    // Handle file upload
    HTTP::StatusCode status;
    bool success = false;

    if (location && !location->uploadStore.empty()) {
        // Use configured upload directory
        std::string uploadDir = location->uploadStore;
        std::string filename = "upload_" + std::to_string(time(nullptr)) + ".txt";
        std::string uploadPath = "/" + filename;
        success = FileUtils::writeFile(uploadDir, uploadPath, request.body, status);
    } else {
        // Fallback to default behavior
        success = FileUtils::writeFile(effectiveRoot, effectiveUri, request.body, status);
    }

    return HTTP::ResponseBuilder::createSimpleResponse(
        status, success ? "File uploaded successfully" : "Upload failed");
}

std::string handleDeleteRequest(const HTTP::Request& request, const std::string& effectiveRoot, 
                              const std::string& effectiveUri, const LocationBlock* location) {
    // Check if DELETE is allowed in this location (SOLID: Open/Closed Principle)
    if (!RequestValidator::isMethodAllowed(request, location)) {
        return HTTP::ResponseBuilder::createErrorResponse(HTTP::StatusCode::METHOD_NOT_ALLOWED);
    }
    
    HTTP::StatusCode status;
    bool success = FileUtils::deleteFile(effectiveRoot, effectiveUri, status);
    
    return HTTP::ResponseBuilder::createSimpleResponse(status, success ? "File deleted" : "Delete failed");
}

// Request processing utilities
std::pair<std::string, std::string> resolveEffectivePath(const std::string& uri, 
                                                        const std::string& rootDirectory, 
                                                        const LocationBlock* location) {
    std::string effectiveRoot = rootDirectory;
    std::string effectiveUri = uri;
    
    if (location && !location->root.empty()) {
        effectiveRoot = location->root;
        
        // Strip the location path from the URI to avoid duplication
        // e.g., URI "/scripts/file.py" with location path "/scripts" becomes "/file.py"
        if (uri.find(location->path) == 0) {
            effectiveUri = uri.substr(location->path.length());
            // Ensure we have a leading slash
            if (effectiveUri.empty() || effectiveUri[0] != '/') {
                effectiveUri = "/" + effectiveUri;
            }
        }
    }
    
    return {effectiveRoot, effectiveUri};
}

// Error handling utilities
std::string handleErrorWithCustomPage(HTTP::StatusCode status, 
                                    const std::map<HTTP::StatusCode, std::string>& customErrorPages,
                                    const std::string& rootDirectory) {
    auto errorPageIt = customErrorPages.find(status);
    if (errorPageIt != customErrorPages.end()) {
        HTTP::StatusCode fileStatus;
        std::string customContent = FileUtils::readFile(rootDirectory, errorPageIt->second, fileStatus);
        if (fileStatus == HTTP::StatusCode::OK) {
            return HTTP::ResponseBuilder::createFileResponse(status, customContent, "text/html");
        }
    }
    return HTTP::ResponseBuilder::createErrorResponse(status);
}

// Method dispatcher using hash map for O(1) lookup
const std::map<HTTP::Method, MethodHandler>& getMethodHandlers() {
    static const std::map<HTTP::Method, MethodHandler> handlers = {
        {HTTP::Method::GET, handleGetRequest},
        {HTTP::Method::POST, handlePostRequest},
        {HTTP::Method::DELETE, handleDeleteRequest}
        // Add more methods as needed: HEAD, PUT, PATCH, etc.
    };
    return handlers;
}

std::string dispatchMethodHandler(const HTTP::Request& request, 
                                const std::string& effectiveRoot,
                                const std::string& effectiveUri, 
                                const LocationBlock* location) {
    const auto& handlers = getMethodHandlers();
    auto it = handlers.find(request.requestLine.method);
    
    if (it != handlers.end()) {
        return it->second(request, effectiveRoot, effectiveUri, location);
    } else {
        return HTTP::ResponseBuilder::createErrorResponse(HTTP::StatusCode::METHOD_NOT_ALLOWED);
    }
}

} // namespace HTTP
