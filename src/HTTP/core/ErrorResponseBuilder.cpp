#include "HTTP/core/ErrorResponseBuilder.hpp"
#include "HTTP/core/HttpResponse.hpp"
#include "utils/Utils.hpp"
#include <fstream>
#include <sstream>

// Static member definition
const ServerBlock* ErrorResponseBuilder::_currentConfig = nullptr;

void ErrorResponseBuilder::setCurrentConfig(const ServerBlock* config) {
    _currentConfig = config;
}

std::string ErrorResponseBuilder::buildResponse(int statusCode) {
    // Try to load custom error page first
    std::string customPage = loadCustomErrorPage(statusCode);
    if (!customPage.empty()) {
        return HttpResponse::buildResponse(statusCode, getStatusText(statusCode), customPage, "text/html");
    }
    
    // Fallback to default error page
    return buildDefaultError(statusCode);
}

std::string ErrorResponseBuilder::loadCustomErrorPage(int statusCode) {
    if (!_currentConfig || _currentConfig->errorPages.find(statusCode) == _currentConfig->errorPages.end()) {
        return "";
    }
    
    std::string errorPagePath = _currentConfig->root + "/" + _currentConfig->errorPages.at(statusCode);
    std::ifstream file(errorPagePath);
    if (!file.is_open()) {
        return "";
    }
    
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string ErrorResponseBuilder::buildDefaultError(int statusCode) {
    std::string statusText = getStatusText(statusCode);
    std::ostringstream content;
    content << "<html><body><h1>" << statusCode << " " << statusText << "</h1></body></html>";
    
    return HttpResponse::buildResponse(statusCode, statusText, content.str(), "text/html");
}

std::string ErrorResponseBuilder::getStatusText(int statusCode) {
    switch (statusCode) {
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        default: return "Error";
    }
}
