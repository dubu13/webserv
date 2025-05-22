#include "HTTPHandler.hpp"
#include <iostream>
#include <fstream>

HTTPHandler::HTTPHandler(const std::string& root) 
    : _root_directory(root), 
      _errorHandler(root),
      _resourceHandler(root, &_errorHandler),
      _cgiHandler(root, &_errorHandler) {
    _handlers[HTTP::Method::GET] = &HTTPHandler::handleGET;
    _handlers[HTTP::Method::POST] = &HTTPHandler::handlePOST;
    _handlers[HTTP::Method::DELETE] = &HTTPHandler::handleDELETE;
}

HTTPHandler::~HTTPHandler() {}

std::unique_ptr<HTTPResponse> HTTPHandler::handleRequest(const std::string& requestData) {
    try {
        auto request = IHTTPRequest::createRequest(requestData);
        
        if (request) {
            std::cout << "Processing " << HTTP::methodToString(request->getMethod()) 
                     << " request for URI: " << request->getUri() << std::endl;
            
            auto handlerIt = _handlers.find(request->getMethod());
            if (handlerIt != _handlers.end()) {
                return (this->*(handlerIt->second))(*request);
            } else {
                return handleError(HTTP::StatusCode::METHOD_NOT_ALLOWED);
            }
        } else {
            return handleError(HTTP::StatusCode::BAD_REQUEST);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return handleError(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
    }
}

std::unique_ptr<HTTPResponse> HTTPHandler::handleGET(const IHTTPRequest& request) {
    std::string uri = request.getUri();
    std::string filePath = _root_directory + uri;
    
    if (_cgiHandler.canHandle(filePath)) {
        return _cgiHandler.executeCGI(uri, request);
    } else {
        return _resourceHandler.serveResource(uri);
    }
}

    }
        return handleError(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
std::unique_ptr<HTTPResponse> HTTPHandler::handleError(HTTP::StatusCode status) {
    return _errorHandler.generateErrorResponse(status);
}

void HTTPHandler::setRootDirectory(const std::string& root) {
    _root_directory = root;
    _resourceHandler.setRootDirectory(root);
    _cgiHandler.setRootDirectory(root);
    _errorHandler.setRootDirectory(root);
}

