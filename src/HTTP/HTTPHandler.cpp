#include "HTTP/HTTPHandler.hpp"
#include "utils/FileUtils.hpp"
#include <iostream>
#include <ctime>

HTTPHandler::HTTPHandler(const std::string &root, const ServerBlock *config)
    : _root_directory(root), _cgiHandler(root), _config(config) {
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
      return HTTP::createErrorResponse(HTTP::StatusCode::BAD_REQUEST);
    }
    std::string uri = request.requestLine.uri;
    
    // Get location configuration for this URI
    const LocationBlock *location = nullptr;
    if (_config) {
      location = _config->getLocation(uri);
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
    
    std::string filePath = _root_directory + uri;
    switch (request.requestLine.method) {
    case HTTP::Method::GET:
      if (_cgiHandler.canHandle(filePath)) {
        std::string cgiResponse = _cgiHandler.executeCGI(uri, request);
        return !cgiResponse.empty()
                   ? cgiResponse
                   : HTTP::createErrorResponse(
                         HTTP::StatusCode::INTERNAL_SERVER_ERROR);
      } else {
        HTTP::StatusCode status;
        std::string content = FileUtils::readFile(_root_directory, uri, status);
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
        std::string cgiResponse = _cgiHandler.executeCGI(uri, request);
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
          success = FileUtils::writeFile(_root_directory, uri, request.body, status);
        }
        
        return HTTP::createSimpleResponse(
            status, success ? "File uploaded successfully" : "Upload failed");
      }
    case HTTP::Method::DELETE: {
      HTTP::StatusCode status;
      bool success = FileUtils::deleteFile(_root_directory, uri, status);
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
    std::cerr << "Server error: " << e.what() << std::endl;
    return HTTP::createErrorResponse(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
  }
}
void HTTPHandler::setRootDirectory(const std::string &root) {
  _root_directory = root;
  _cgiHandler.setRootDirectory(root);
}
