#include "HTTP/HTTPHandler.hpp"
#include "utils/FileUtils.hpp"
#include "HTTP/HttpResponseBuilder.hpp"
#include "utils/Logger.hpp"
#include <iostream>
#include <ctime>

// Include the handler implementations
#include "HTTP/HTTPHandlers.ipp"

HTTPHandler::HTTPHandler(const std::string &root, const ServerBlock *config)
    : _root_directory(root), _cgiHandler(root), _config(config) {
  Logger::infof("HTTPHandler initialized with root: %s", root.c_str());
  if (_config && !_config->errorPages.empty()) {
    for (const auto &errorPage : _config->errorPages) {
      _custom_error_pages[static_cast<HTTP::StatusCode>(errorPage.first)] =
          errorPage.second;
    }
  } else {
    _custom_error_pages[HTTP::StatusCode::NOT_FOUND] = "/errors/404.html";
    _custom_error_pages[HTTP::StatusCode::BAD_REQUEST] = "/errors/400.html";
    _custom_error_pages[HTTP::StatusCode::INTERNAL_SERVER_ERROR] =
        "/errors/500.html";
    _custom_error_pages[HTTP::StatusCode::METHOD_NOT_ALLOWED] =
        "/errors/405.html";
    _custom_error_pages[HTTP::StatusCode::PAYLOAD_TOO_LARGE] =
        "/errors/413.html";
  }
}
HTTPHandler::~HTTPHandler() {}

std::string HTTPHandler::handleRequest(const std::string &requestData) {
  Logger::debugf("HTTPHandler processing request (%zu bytes)", requestData.size());
  
  try {
    HttpParser::Request request;
    if (!HttpParser::parseRequest(requestData, request)) {
      Logger::warn("Failed to parse HTTP request - malformed request");
      Logger::debugf("Raw request data: %s", requestData.substr(0, std::min(requestData.size(), size_t(200))).c_str());
      return createErrorResponse(HTTP::StatusCode::BAD_REQUEST);
    }
    
    std::string uri = request.requestLine.uri;
    std::string methodStr = HTTP::methodToString(request.requestLine.method);
    Logger::infof("Processing %s request for URI: %s", methodStr.c_str(), uri.c_str());
    
    #ifdef DEBUG
    // Log important headers
    for (const auto& [key, value] : request.headers) {
      if (key == "Host" || key == "Content-Length" || key == "Content-Type" || key == "Connection") {
        Logger::debugf("Header: %s: %s", key.c_str(), value.c_str());
      }
    }
    
    if (!request.body.empty()) {
      Logger::debugf("Request body length: %zu bytes", request.body.length());
    }
    #endif

    // Get location configuration for this URI
    const LocationBlock *location = nullptr;
    if (_config) {
      location = _config->getLocation(uri);
      Logger::debugf("Location lookup for URI '%s': %s", uri.c_str(), 
                     location ? "found" : "not found");
      if (location) {
        Logger::debugf("Location details - path: '%s', autoindex: %s, index: '%s'",
                       location->path.c_str(), 
                       location->autoindex ? "enabled" : "disabled",
                       location->index.c_str());
      }
    } else {
      Logger::warn("No server configuration available for location lookup");
    }
    
    // Check for redirections first (highest priority)
    if (location && !location->redirection.empty()) {
      Logger::infof("Redirect found for URI %s: %s", uri.c_str(), location->redirection.c_str());
      
      // Parse redirection string: "301 /new/path" or just "/new/path" (defaults to 301)
      std::string redirStr = location->redirection;
      HTTP::StatusCode redirectStatus = HTTP::StatusCode::MOVED_PERMANENTLY; // Default 301
      std::string redirectLocation;
      
      size_t spacePos = redirStr.find(' ');
      if (spacePos != std::string::npos) {
        // Format: "301 /new/path"
        std::string statusStr = redirStr.substr(0, spacePos);
        redirectLocation = redirStr.substr(spacePos + 1);
        
        try {
          int statusCode = std::stoi(statusStr);
          if (statusCode == 301) {
            redirectStatus = HTTP::StatusCode::MOVED_PERMANENTLY;
          } else if (statusCode == 302) {
            redirectStatus = HTTP::StatusCode::FOUND;
          } else {
            Logger::warnf("Unsupported redirect status code %d, using 301", statusCode);
          }
        } catch (const std::exception& e) {
          Logger::warnf("Invalid redirect status code '%s', using 301: %s", statusStr.c_str(), e.what());
        }
      } else {
        // Format: "/new/path" (default to 301)
        redirectLocation = redirStr;
      }
      
      Logger::infof("Redirecting %s to %s with status %d", uri.c_str(), redirectLocation.c_str(), static_cast<int>(redirectStatus));
      return HttpResponseBuilder::createRedirectResponse(redirectStatus, redirectLocation);
    }
    
    // Resolve effective paths
    auto [effectiveRoot, effectiveUri] = HTTP::resolveEffectivePath(uri, _root_directory, location);
    Logger::debugf("Resolved paths - root: %s, uri: %s", effectiveRoot.c_str(), effectiveUri.c_str());

    // Validate client body size limits
    size_t maxBodySize = _config ? _config->clientMaxBodySize : 1024 * 1024; // Default 1MB
    if (location && location->clientMaxBodySize > 0) {
      maxBodySize = location->clientMaxBodySize;
      Logger::debugf("Using location-specific body size limit: %zu bytes", maxBodySize);
    } else {
      Logger::debugf("Using server-level body size limit: %zu bytes", maxBodySize);
    }
    
    if (request.body.size() > maxBodySize) {
      Logger::warnf("Request body too large: %zu bytes (limit: %zu bytes)", 
                    request.body.size(), maxBodySize);
      return createErrorResponse(HTTP::StatusCode::PAYLOAD_TOO_LARGE);
    }

    // Validate method for location
    if (!HTTP::RequestValidator::isMethodAllowed(request, location)) {
      return createErrorResponse(HTTP::StatusCode::METHOD_NOT_ALLOWED);
    }

    // Handle CGI requests first (if applicable)
    std::string filePath = effectiveRoot + effectiveUri;
    if (_cgiHandler.canHandle(filePath)) {
      return handleCgiRequest(request, effectiveRoot, effectiveUri);
    }

    // Dispatch to method-specific handlers using hash lookup
    return HTTP::dispatchMethodHandler(request, effectiveRoot, effectiveUri, location);
    
  } catch (const std::exception &e) {
    Logger::errorf("HTTPHandler error: %s", e.what());
    return createErrorResponse(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
  }
}

std::string HTTPHandler::handleCgiRequest(const HttpParser::Request& request, const std::string& effectiveRoot, const std::string& effectiveUri) {
    Logger::infof("Handling CGI request for: %s", effectiveUri.c_str());
    
    // Temporarily set CGI handler root for this request
    std::string originalRoot = _root_directory;
    _cgiHandler.setRootDirectory(effectiveRoot);
    std::string cgiResponse = _cgiHandler.executeCGI(effectiveUri, request);
    _cgiHandler.setRootDirectory(originalRoot);  // Restore original root
    
    Logger::debugf("CGI response length: %zu bytes", cgiResponse.length());
    return !cgiResponse.empty()
               ? cgiResponse
               : createErrorResponse(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
  }

void HTTPHandler::setRootDirectory(const std::string &root) {
  Logger::infof("HTTPHandler root directory changed: %s -> %s", _root_directory.c_str(), root.c_str());
  _root_directory = root;
  _cgiHandler.setRootDirectory(root);
}

std::string HTTPHandler::createErrorResponse(HTTP::StatusCode status, const std::string& message) {
  Logger::debugf("HTTPHandler::createErrorResponse - Creating error response with status %d", 
                 static_cast<int>(status));
  
  // Try to use custom error page if available
  auto errorPageIt = _custom_error_pages.find(status);
  if (errorPageIt != _custom_error_pages.end()) {
    Logger::debugf("Using custom error page: %s", errorPageIt->second.c_str());
    
    HTTP::StatusCode fileStatus;
    std::string customContent = FileUtils::readFile(_root_directory, errorPageIt->second, fileStatus);
    
    if (fileStatus == HTTP::StatusCode::OK) {
      Logger::debug("Custom error page loaded successfully");
      return HttpResponseBuilder::createFileResponse(status, customContent, "text/html");
    } else {
      Logger::warnf("Custom error page failed to load from %s, using default", errorPageIt->second.c_str());
    }
  }
  
  // Fallback to default error response
  return HttpResponseBuilder::createErrorResponse(status, message);
}
