#include "HTTP/HTTPHandler.hpp"
#include "utils/FileUtils.hpp"
#include "HTTP/HTTPResponseBuilder.hpp"
#include "utils/Logger.hpp"
#include <iostream>
#include <ctime>

// Include the handler implementations
#include "HTTP/HTTPHandlers.hpp"

HTTPHandler::HTTPHandler(const std::string& root, const ServerBlock* config)
    : _root_directory(root), _cgiHandler(root), _config(config) {
    if (_config && !_config->errorPages.empty()) {
        for (const auto& errorPage : _config->errorPages) {
            _custom_error_pages[static_cast<HTTP::StatusCode>(errorPage.first)] = errorPage.second;
        }
    } else {
        _custom_error_pages[HTTP::StatusCode::NOT_FOUND] = "/errors/404.html";
        _custom_error_pages[HTTP::StatusCode::BAD_REQUEST] = "/errors/400.html";
        _custom_error_pages[HTTP::StatusCode::INTERNAL_SERVER_ERROR] = "/errors/500.html";
        _custom_error_pages[HTTP::StatusCode::METHOD_NOT_ALLOWED] = "/errors/405.html";
        _custom_error_pages[HTTP::StatusCode::PAYLOAD_TOO_LARGE] = "/errors/413.html";
    }
}

HTTPHandler::~HTTPHandler() {}

std::string HTTPHandler::handleRequest(const std::string& requestData) {
    try {
        HTTP::Request request;
        if (!HTTP::Parser::parseRequest(requestData, request)) {
            return createErrorResponse(HTTP::StatusCode::BAD_REQUEST);
        }
        
        std::string uri = request.requestLine.uri;
        
        const LocationBlock* location = nullptr;
        if (_config) {
            location = _config->getLocation(uri);
        }
        
        if (location && !location->redirection.empty()) {
            std::string redirStr = location->redirection;
            HTTP::StatusCode redirectStatus = HTTP::StatusCode::MOVED_PERMANENTLY;
            std::string redirectLocation;
            
            size_t spacePos = redirStr.find(' ');
            if (spacePos != std::string::npos) {
                std::string statusStr = redirStr.substr(0, spacePos);
                redirectLocation = redirStr.substr(spacePos + 1);
                
                try {
                    int statusCode = std::stoi(statusStr);
                    if (statusCode == 301) {
                        redirectStatus = HTTP::StatusCode::MOVED_PERMANENTLY;
                    } else if (statusCode == 302) {
                        redirectStatus = HTTP::StatusCode::FOUND;
                    }
                } catch (const std::exception& e) {
                    // Use default 301
                }
            } else {
                redirectLocation = redirStr;
            }
            
            return HTTP::ResponseBuilder::createRedirectResponse(redirectStatus, redirectLocation);
        }
        
        auto [effectiveRoot, effectiveUri] = HTTP::resolveEffectivePath(uri, _root_directory, location);

        size_t maxBodySize = _config ? _config->clientMaxBodySize : 1024 * 1024;
        if (location && location->clientMaxBodySize > 0) {
            maxBodySize = location->clientMaxBodySize;
        }
        
        if (request.body.size() > maxBodySize) {
            return createErrorResponse(HTTP::StatusCode::PAYLOAD_TOO_LARGE);
        }

        if (!HTTP::RequestValidator::isMethodAllowed(request, location)) {
            return createErrorResponse(HTTP::StatusCode::METHOD_NOT_ALLOWED);
        }

        std::string filePath = effectiveRoot + effectiveUri;
        if (_cgiHandler.canHandle(filePath)) {
            return handleCgiRequest(request, effectiveRoot, effectiveUri);
        }

        std::string response = HTTP::dispatchMethodHandler(request, effectiveRoot, effectiveUri, location);
        
        HTTP::StatusCode responseStatus = extractStatusFromResponse(response);
        if (isErrorStatus(responseStatus) && isDefaultErrorResponse(response)) {
            return createErrorResponse(responseStatus);
        }
        
        return response;
        
    } catch (const std::exception &e) {
        return createErrorResponse(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
    }
}

std::string HTTPHandler::handleCgiRequest(const HTTP::Request& request, const std::string& effectiveRoot, const std::string& effectiveUri) {
    std::string originalRoot = _root_directory;
    _cgiHandler.setRootDirectory(effectiveRoot);
    std::string cgiResponse = _cgiHandler.executeCGI(effectiveUri, request);
    _cgiHandler.setRootDirectory(originalRoot);
    
    return !cgiResponse.empty()
               ? cgiResponse
               : createErrorResponse(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
}

void HTTPHandler::setRootDirectory(const std::string &root) {
    _root_directory = root;
    _cgiHandler.setRootDirectory(root);
}

std::string HTTPHandler::createErrorResponse(HTTP::StatusCode status, const std::string& message) {
    auto errorPageIt = _custom_error_pages.find(status);
    if (errorPageIt != _custom_error_pages.end()) {
        HTTP::StatusCode fileStatus;
        std::string customContent = FileUtils::readFile(_root_directory, errorPageIt->second, fileStatus);
        
        if (fileStatus == HTTP::StatusCode::OK) {
            return HTTP::ResponseBuilder::createFileResponse(status, customContent, "text/html");
        }
    }
    
    return HTTP::ResponseBuilder::createErrorResponse(status, message);
}

HTTP::StatusCode HTTPHandler::extractStatusFromResponse(const std::string& response) const {
  // Extract status code from HTTP response first line
  // Format: "HTTP/1.1 404 Not Found\r\n..."
  if (response.size() < 12) return HTTP::StatusCode::INTERNAL_SERVER_ERROR;
  
  size_t firstSpace = response.find(' ');
  if (firstSpace == std::string::npos || firstSpace + 4 >= response.size()) {
    return HTTP::StatusCode::INTERNAL_SERVER_ERROR;
  }
  
  std::string statusStr = response.substr(firstSpace + 1, 3);
  try {
    int statusCode = std::stoi(statusStr);
    return static_cast<HTTP::StatusCode>(statusCode);
  } catch (const std::exception&) {
    return HTTP::StatusCode::INTERNAL_SERVER_ERROR;
  }
}

bool HTTPHandler::isErrorStatus(HTTP::StatusCode status) const {
    int code = static_cast<int>(status);
    return code >= 400 && code < 600;
}

bool HTTPHandler::isDefaultErrorResponse(const std::string& response) const {
    // Check if response contains the default error HTML pattern
    // Default error responses contain: "<title>Error XXX</title>"
    return response.find("<title>Error ") != std::string::npos &&
           response.find("</title></head><body><h1>Error ") != std::string::npos;
}
