#include "resource/ResourceHandler.hpp"
#include <fstream>
#include <vector>
#include <iostream>

ResourceHandler::ResourceHandler(const std::string& root, ErrorHandler* errorHandler) 
    : _root_directory(root), _errorHandler(errorHandler) {}

ResourceHandler::~ResourceHandler() {}

std::unique_ptr<HTTPResponse> ResourceHandler::serveResource(const std::string& uri) {
    auto response = HTTPResponse::createResponse();
    
    std::string normalizedUri = uri;
    if (normalizedUri == "/") {
        normalizedUri = "/index.html";
    }
    
    std::string filePath = _root_directory + normalizedUri;
    
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    
    if (file.is_open()) {
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<char> buffer(size);
        if (file.read(buffer.data(), size)) {
            std::string contentType = FileType::getMimeType(filePath);
            
            response->setStatus(HTTP::StatusCode::OK);
            response->setContentType(contentType);
            response->setBody(std::string(buffer.data(), size));
        } else {
            // Error reading file
            if (_errorHandler) {
                return _errorHandler->generateErrorResponse(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
            } else {
                response->setStatus(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
                response->setContentType("text/html");
                response->setBody("<html><body><h1>500 Internal Server Error</h1><p>Failed to read file</p></body></html>");
            }
        }
    } else {
        if (_errorHandler) {
            return _errorHandler->generateErrorResponse(HTTP::StatusCode::NOT_FOUND);
        } else {
            response->setStatus(HTTP::StatusCode::NOT_FOUND);
            response->setContentType("text/html");
            response->setBody("<html><body><h1>404 Not Found</h1><p>The requested resource was not found on this server</p></body></html>");
        }
    }
    
    response->setHeader("Connection", "close");
    
    return response;
}

void ResourceHandler::setRootDirectory(const std::string& root) {
    _root_directory = root;
}

void ResourceHandler::setErrorHandler(ErrorHandler* errorHandler) {
    _errorHandler = errorHandler;
}

std::string ResourceHandler::getRootDirectory() const {
    return _root_directory;
}