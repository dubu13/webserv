#include "HTTP/HTTPHandler.hpp"
#include "utils/FileUtils.hpp"
#include "utils/Logger.hpp"
#include <iostream>
#include <ctime>

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
  try {
    HTTP::Request request;
    if (!HTTP::parseRequest(requestData, request)) {
      Logger::warn("Failed to parse HTTP request");
      return HTTP::createErrorResponse(HTTP::StatusCode::BAD_REQUEST);
    }
    std::string uri = request.requestLine.uri;
    Logger::infof("Processing %s request for URI: %s",
                  HTTP::methodToString(request.requestLine.method).c_str(), uri.c_str());

    // Get location configuration for this URI
    const LocationBlock *location = nullptr;
    std::string effectiveRoot = _root_directory;
    std::string effectiveUri = uri;
    
    if (_config) {
      location = _config->getLocation(uri);
      if (location && !location->root.empty()) {
        // Use location-specific root and strip location path from URI
        effectiveRoot = location->root;
        
        // Find the location path by matching against all configured locations
        std::string locationPath = "";
        for (const auto& [path, config] : _config->locations) {
          if (uri.find(path) == 0 && path.length() > locationPath.length()) {
            locationPath = path;
          }
        }
        
        // Strip location path from URI for location-specific root
        if (!locationPath.empty() && locationPath != "/") {
          effectiveUri = uri.substr(locationPath.length());
          if (effectiveUri.empty()) {
            effectiveUri = "/";
          }
        }
      }
    }

    // Check if method is allowed for this location
    if (location && !location->allowedMethods.empty()) {
      std::string methodStr;
      switch (request.requestLine.method) {
        case HTTP::Method::GET: methodStr = "GET"; break;
        case HTTP::Method::POST: methodStr = "POST"; break;
        case HTTP::Method::DELETE: methodStr = "DELETE"; break;
        default: methodStr = "UNKNOWN"; break;
      }

      if (location->allowedMethods.find(methodStr) == location->allowedMethods.end()) {
        return HTTP::createErrorResponse(HTTP::StatusCode::METHOD_NOT_ALLOWED);
      }
    }

    std::string filePath = effectiveRoot + effectiveUri;
    Logger::debugf("Attempting to access file: %s (root: %s, uri: %s, effectiveRoot: %s, effectiveUri: %s)", 
                   filePath.c_str(), _root_directory.c_str(), uri.c_str(), effectiveRoot.c_str(), effectiveUri.c_str());
    
    switch (request.requestLine.method) {
    case HTTP::Method::GET:
      if (_cgiHandler.canHandle(filePath)) {
        Logger::debug("Handling CGI request");
        // Temporarily set CGI handler root for this request
        std::string originalRoot = _root_directory;
        _cgiHandler.setRootDirectory(effectiveRoot);
        std::string cgiResponse = _cgiHandler.executeCGI(effectiveUri, request);
        _cgiHandler.setRootDirectory(originalRoot);  // Restore original root
        return !cgiResponse.empty()
                   ? cgiResponse
                   : HTTP::createErrorResponse(
                         HTTP::StatusCode::INTERNAL_SERVER_ERROR);
      } else {
        Logger::debugf("Reading file from: %s", filePath.c_str());
        HTTP::StatusCode status;
        std::string content = FileUtils::readFile(effectiveRoot, effectiveUri, status);
        if (status != HTTP::StatusCode::OK) {
          auto errorPageIt = _custom_error_pages.find(status);
          if (errorPageIt != _custom_error_pages.end()) {
            HTTP::StatusCode fileStatus;
            std::string customContent = FileUtils::readFile(
                _root_directory, errorPageIt->second, fileStatus);
            if (fileStatus == HTTP::StatusCode::OK) {
              return HTTP::createFileResponse(status, customContent,
                                              "text/html");
            }
          }
          return HTTP::createErrorResponse(status);
        }
        return HTTP::createFileResponse(status, content,
                                        HTTP::getMimeType(filePath));
      }
      break;
    case HTTP::Method::POST:
      if (_cgiHandler.canHandle(filePath)) {
        // Temporarily set CGI handler root for this request
        std::string originalRoot = _root_directory;
        _cgiHandler.setRootDirectory(effectiveRoot);
        std::string cgiResponse = _cgiHandler.executeCGI(effectiveUri, request);
        _cgiHandler.setRootDirectory(originalRoot);  // Restore original root
        return !cgiResponse.empty()
                   ? cgiResponse
                   : HTTP::createErrorResponse(
                         HTTP::StatusCode::INTERNAL_SERVER_ERROR);
      } else {
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

        return HTTP::createSimpleResponse(
            status, success ? "File uploaded successfully" : "Upload failed");
      }
    case HTTP::Method::DELETE: {
      HTTP::StatusCode status;
      bool success = FileUtils::deleteFile(effectiveRoot, effectiveUri, status);
      return HTTP::createSimpleResponse(status, success ? "File deleted"
                                                        : "Delete failed");
    }
    case HTTP::Method::HEAD:
    case HTTP::Method::PUT:
    case HTTP::Method::PATCH:
    default:
      return HTTP::createErrorResponse(HTTP::StatusCode::METHOD_NOT_ALLOWED);
    }
  } catch (const std::exception &e) {
    Logger::errorf("Server error: %s", e.what());
    return HTTP::createErrorResponse(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
  }
}
void HTTPHandler::setRootDirectory(const std::string &root) {
  _root_directory = root;
  _cgiHandler.setRootDirectory(root);
}
