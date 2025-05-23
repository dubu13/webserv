#pragma once

#include "HTTP/HTTPResponse.hpp"
#include "HTTP/HTTPTypes.hpp"
#include <string>
#include <memory>
#include <map>

class ErrorHandler {
private:
    std::string _root_directory;
    std::map<HTTP::StatusCode, std::string> _custom_error_pages;
    
    // Get generic fallback error message for a status code
    std::string getGenericErrorMessage(HTTP::StatusCode status) const;

public:
    ErrorHandler(const std::string& root = "./www");
    ~ErrorHandler();
    
    // Main method to generate an error response based on status code
    std::unique_ptr<HTTPResponse> generateErrorResponse(HTTP::StatusCode status);
    
    // Configure a custom error page for a specific status code
    void setCustomErrorPage(HTTP::StatusCode status, const std::string& path);
    
    // Check if a custom error page exists for a status code
    bool hasCustomErrorPage(HTTP::StatusCode status) const;
    
    // Setters and getters
    void setRootDirectory(const std::string& root);
    std::string getRootDirectory() const;
};