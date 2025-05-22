#include "ErrorHandler.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

ErrorHandler::ErrorHandler(const std::string& root) : _root_directory(root) {
    // Set up default error page mappings using relative paths
    _custom_error_pages[HTTP::StatusCode::NOT_FOUND] = "/errors/404.html";
    _custom_error_pages[HTTP::StatusCode::BAD_REQUEST] = "/errors/400.html";
    _custom_error_pages[HTTP::StatusCode::INTERNAL_SERVER_ERROR] = "/errors/500.html";
    _custom_error_pages[HTTP::StatusCode::METHOD_NOT_ALLOWED] = "/errors/405.html"; // If you have this
}

ErrorHandler::~ErrorHandler() {}

std::unique_ptr<HTTPResponse> ErrorHandler::generateErrorResponse(HTTP::StatusCode status) {
    auto response = HTTPResponse::createResponse();
    response->setStatus(status);
    response->setContentType("text/html");
    response->setHeader("Connection", "close");
    
    // Determine if we have a custom error page for this status
    std::string errorContent;
    if (hasCustomErrorPage(status)) {
        // Attempt to load the custom error page
        std::string errorFilePath = _root_directory + _custom_error_pages[status];
        std::ifstream errorFile(errorFilePath);
        
        if (errorFile.is_open()) {
            std::stringstream buffer;
            buffer << errorFile.rdbuf();
            errorContent = buffer.str();
        } else {
            // If we can't open the custom page, fall back to generic message
            errorContent = getGenericErrorMessage(status);
            std::cerr << "Warning: Could not open custom error page: " << errorFilePath << std::endl;
        }
    } else {
        // No custom page configured, use the generic message
        errorContent = getGenericErrorMessage(status);
    }
    
    response->setBody(errorContent);
    return response;
}

std::string ErrorHandler::getGenericErrorMessage(HTTP::StatusCode status) const {
    std::string statusStr = std::to_string(static_cast<int>(status));
    std::string statusText = HTTP::statusToString(status);
    
    return "<html><body><h1>" + statusStr + " " + statusText + "</h1>" +
           "<p>The server encountered an error processing your request.</p>" +
           "</body></html>";
}

void ErrorHandler::setCustomErrorPage(HTTP::StatusCode status, const std::string& path) {
    _custom_error_pages[status] = path;
}

bool ErrorHandler::hasCustomErrorPage(HTTP::StatusCode status) const {
    return _custom_error_pages.find(status) != _custom_error_pages.end();
}

void ErrorHandler::setRootDirectory(const std::string& root) {
    _root_directory = root;
}

std::string ErrorHandler::getRootDirectory() const {
    return _root_directory;
}