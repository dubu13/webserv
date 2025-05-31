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
      return HttpResponseBuilder::createErrorResponse(HTTP::StatusCode::BAD_REQUEST);
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
    
    // Resolve effective paths
    auto [effectiveRoot, effectiveUri] = HTTP::resolveEffectivePath(uri, _root_directory, location);
    Logger::debugf("Resolved paths - root: %s, uri: %s", effectiveRoot.c_str(), effectiveUri.c_str());

    // Validate method for location
    if (!HTTP::RequestValidator::isMethodAllowed(request, location)) {
      return HttpResponseBuilder::createErrorResponse(HTTP::StatusCode::METHOD_NOT_ALLOWED);
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
    return HttpResponseBuilder::createErrorResponse(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
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
               : HttpResponseBuilder::createErrorResponse(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
  }

void HTTPHandler::setRootDirectory(const std::string &root) {
  Logger::infof("HTTPHandler root directory changed: %s -> %s", _root_directory.c_str(), root.c_str());
  _root_directory = root;
  _cgiHandler.setRootDirectory(root);
}
