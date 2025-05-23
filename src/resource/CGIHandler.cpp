#include "CGIHandler.hpp"
#include <iostream>

CGIHandler::CGIHandler(const std::string& root, ErrorHandler* errorHandler) 
    : _root_directory(root), _errorHandler(errorHandler), _executor(errorHandler) {
    registerHandler(".php", "/usr/bin/php");
    registerHandler(".py", "/usr/bin/python");
    registerHandler(".pl", "/usr/bin/perl");
}

CGIHandler::~CGIHandler() {}

std::unique_ptr<HTTPResponse> CGIHandler::executeCGI(const std::string& uri, const IHTTPRequest& request) {
    std::string filePath = _root_directory + uri;
    
    size_t dot_pos = filePath.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return _executor.createErrorResponse(HTTP::StatusCode::BAD_REQUEST, "Invalid CGI filename");
    }
    
    std::string extension = filePath.substr(dot_pos);
    
    auto handlerIt = _cgi_handlers.find(extension);
    if (handlerIt == _cgi_handlers.end()) {
        return _executor.createErrorResponse(HTTP::StatusCode::INTERNAL_SERVER_ERROR, 
                                           "No handler registered for " + extension + " scripts");
    }
    
    return _executor.execute(filePath, handlerIt->second, request);
}

void CGIHandler::registerHandler(const std::string& extension, const std::string& handlerPath) {
    _cgi_handlers[extension] = handlerPath;
}

void CGIHandler::unregisterHandler(const std::string& extension) {
    _cgi_handlers.erase(extension);
}

bool CGIHandler::canHandle(const std::string& filePath) const {
    size_t dot_pos = filePath.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return false;
    }
    
    std::string extension = filePath.substr(dot_pos);
    return _cgi_handlers.find(extension) != _cgi_handlers.end();
}

void CGIHandler::setRootDirectory(const std::string& root) {
    _root_directory = root;
}

void CGIHandler::setErrorHandler(ErrorHandler* errorHandler) {
    _errorHandler = errorHandler;
    _executor.setErrorHandler(errorHandler);
}