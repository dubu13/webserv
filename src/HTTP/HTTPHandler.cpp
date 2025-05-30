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
  Logger::debugf("HTTPHandler processing request (%zu bytes)", requestData.size());
  
  try {
    HTTP::Request request;
    if (!HTTP::parseRequest(requestData, request)) {
      Logger::warn("Failed to parse HTTP request - malformed request");
      Logger::debugf("Raw request data: %s", requestData.substr(0, std::min(requestData.size(), size_t(200))).c_str());
      return HTTP::createErrorResponse(HTTP::StatusCode::BAD_REQUEST);
    }
    
    std::string uri = request.requestLine.uri;
    std::string methodStr = HTTP::methodToString(request.requestLine.method);
    Logger::infof("Processing %s request for URI: %s", methodStr.c_str(), uri.c_str());
    
    // Log important headers
    for (const auto& [key, value] : request.headers) {
      if (key == "Host" || key == "Content-Length" || key == "Content-Type" || key == "Connection") {
        Logger::debugf("Header: %s: %s", key.c_str(), value.c_str());
      }
    }
    
    if (!request.body.empty()) {
      Logger::debugf("Request body length: %zu bytes", request.body.length());
    }

    // Get location configuration for this URI
    const LocationBlock *location = nullptr;
    std::string effectiveRoot = _root_directory;
    std::string effectiveUri = uri;
    
    if (_config) {
      location = _config->getLocation(uri);
      if (location && !location->root.empty()) {
        Logger::debugf("Found location block for URI %s with root: %s", uri.c_str(), location->root.c_str());
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
          Logger::debugf("Stripped location path '%s' from URI, new URI: %s", locationPath.c_str(), effectiveUri.c_str());
        }
      } else {
        Logger::debugf("No specific location found for URI %s, using default root", uri.c_str());
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
        Logger::warnf("Method %s not allowed for URI %s", methodStr.c_str(), uri.c_str());
        return HTTP::createErrorResponse(HTTP::StatusCode::METHOD_NOT_ALLOWED);
      }
    }

    std::string filePath = effectiveRoot + effectiveUri;
    Logger::debugf("Final file path: %s (root: %s, uri: %s)", filePath.c_str(), effectiveRoot.c_str(), effectiveUri.c_str());
    
    switch (request.requestLine.method) {
    case HTTP::Method::GET:
      Logger::debug("Processing GET request");
      if (_cgiHandler.canHandle(filePath)) {
        Logger::infof("Handling CGI request for: %s", filePath.c_str());
        // Temporarily set CGI handler root for this request
        std::string originalRoot = _root_directory;
        _cgiHandler.setRootDirectory(effectiveRoot);
        std::string cgiResponse = _cgiHandler.executeCGI(effectiveUri, request);
        _cgiHandler.setRootDirectory(originalRoot);  // Restore original root
        Logger::debugf("CGI response length: %zu bytes", cgiResponse.length());
        return !cgiResponse.empty()
                   ? cgiResponse
                   : HTTP::createErrorResponse(
                         HTTP::StatusCode::INTERNAL_SERVER_ERROR);
      } else {
        Logger::debugf("Reading static file from: %s", filePath.c_str());
        HTTP::StatusCode status;
        std::string content = FileUtils::readFile(effectiveRoot, effectiveUri, status);
        Logger::debugf("File read result: status=%d, content_length=%zu", static_cast<int>(status), content.length());
        
        if (status != HTTP::StatusCode::OK) {
          Logger::warnf("File access failed with status %d for path: %s", static_cast<int>(status), filePath.c_str());
          auto errorPageIt = _custom_error_pages.find(status);
          if (errorPageIt != _custom_error_pages.end()) {
            Logger::debugf("Using custom error page: %s", errorPageIt->second.c_str());
            HTTP::StatusCode fileStatus;
            std::string customContent = FileUtils::readFile(
                _root_directory, errorPageIt->second, fileStatus);
            if (fileStatus == HTTP::StatusCode::OK) {
              Logger::debug("Custom error page loaded successfully");
              return HTTP::createFileResponse(status, customContent,
                                              "text/html");
            } else {
              Logger::debug("Custom error page failed to load, using default");
            }
          }
          return HTTP::createErrorResponse(status);
        }
        
        std::string mimeType = HTTP::getMimeType(filePath);
        Logger::debugf("Serving file with MIME type: %s", mimeType.c_str());
        return HTTP::createFileResponse(status, content, mimeType);
      }
      break;
    case HTTP::Method::POST:
      Logger::debug("Processing POST request");
      if (_cgiHandler.canHandle(filePath)) {
        Logger::infof("Handling CGI POST request for: %s", filePath.c_str());
        // Temporarily set CGI handler root for this request
        std::string originalRoot = _root_directory;
        _cgiHandler.setRootDirectory(effectiveRoot);
        std::string cgiResponse = _cgiHandler.executeCGI(effectiveUri, request);
        _cgiHandler.setRootDirectory(originalRoot);  // Restore original root
        Logger::debugf("CGI POST response length: %zu bytes", cgiResponse.length());
        return !cgiResponse.empty()
                   ? cgiResponse
                   : HTTP::createErrorResponse(
                         HTTP::StatusCode::INTERNAL_SERVER_ERROR);
      } else {
        Logger::debug("Processing file upload");
        // Handle file upload
        HTTP::StatusCode status;
        bool success = false;

        if (location && !location->uploadStore.empty()) {
          // Use configured upload directory
          std::string uploadDir = location->uploadStore;
          std::string filename = "upload_" + std::to_string(time(nullptr)) + ".txt";
          std::string uploadPath = "/" + filename;
          Logger::debugf("Uploading to configured directory: %s%s", uploadDir.c_str(), uploadPath.c_str());
          success = FileUtils::writeFile(uploadDir, uploadPath, request.body, status);
        } else {
          // Fallback to default behavior
          Logger::debugf("Uploading to default location: %s", filePath.c_str());
          success = FileUtils::writeFile(effectiveRoot, effectiveUri, request.body, status);
        }

        Logger::debugf("Upload result: success=%s, status=%d", success ? "true" : "false", static_cast<int>(status));
        return HTTP::createSimpleResponse(
            status, success ? "File uploaded successfully" : "Upload failed");
      }
    case HTTP::Method::DELETE: {
      Logger::debugf("Processing DELETE request for: %s", filePath.c_str());
      HTTP::StatusCode status;
      bool success = FileUtils::deleteFile(effectiveRoot, effectiveUri, status);
      Logger::debugf("Delete result: success=%s, status=%d", success ? "true" : "false", static_cast<int>(status));
      return HTTP::createSimpleResponse(status, success ? "File deleted"
                                                        : "Delete failed");
    }
    case HTTP::Method::HEAD:
    case HTTP::Method::PUT:
    case HTTP::Method::PATCH:
    default:
      Logger::warnf("Method not allowed: %s", methodStr.c_str());
      return HTTP::createErrorResponse(HTTP::StatusCode::METHOD_NOT_ALLOWED);
    }
  } catch (const std::exception &e) {
    Logger::errorf("HTTPHandler error: %s", e.what());
    return HTTP::createErrorResponse(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
  }
}
void HTTPHandler::setRootDirectory(const std::string &root) {
  _root_directory = root;
  _cgiHandler.setRootDirectory(root);
}
