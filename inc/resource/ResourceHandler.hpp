#pragma once

#include "HTTPResponse.hpp"
#include "FileType.hpp"
#include "ErrorHandler.hpp"
#include <memory>
#include <string>

class ResourceHandler {
private:
    std::string _root_directory;
    ErrorHandler* _errorHandler;
    
public:
    ResourceHandler(const std::string& root = "./www", ErrorHandler* errorHandler = nullptr);
    ~ResourceHandler();
    
    // Core method for handling static file resources
    std::unique_ptr<HTTPResponse> serveResource(const std::string& uri);
    
    // Setters
    void setRootDirectory(const std::string& root);
    void setErrorHandler(ErrorHandler* errorHandler);
    
    // Getters
    std::string getRootDirectory() const;
};