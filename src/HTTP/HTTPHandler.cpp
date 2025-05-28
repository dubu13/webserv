#include "HTTP/HTTPHandler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

// Simple MIME type mapping
static std::string getMimeType(const std::string& path) {
    std::string ext;
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        ext = path.substr(dot_pos + 1);
    }
    
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "txt") return "text/plain";
    if (ext == "pdf") return "application/pdf";
    return "application/octet-stream";
}

HTTPHandler::HTTPHandler(const std::string& root)
    : _root_directory(root),
      _cgiHandler(root) {
    // Initialize custom error pages
    _custom_error_pages[HTTP::StatusCode::NOT_FOUND] = "/errors/404.html";
    _custom_error_pages[HTTP::StatusCode::BAD_REQUEST] = "/errors/400.html";
    _custom_error_pages[HTTP::StatusCode::INTERNAL_SERVER_ERROR] = "/errors/500.html";
    _custom_error_pages[HTTP::StatusCode::METHOD_NOT_ALLOWED] = "/errors/405.html";
}
HTTPHandler::~HTTPHandler() {}
std::unique_ptr<HTTPResponse> HTTPHandler::handleRequest(const std::string& requestData) {
    try {
        HTTPRequest request;
        if (request.parseRequest(requestData)) {
            switch (request.getMethod()) {
                case HTTP::Method::GET:
                    return handleGET(request);
                case HTTP::Method::POST:
                    return handlePOST(request);
                case HTTP::Method::DELETE:
                    return handleDELETE(request);
                default:
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
std::unique_ptr<HTTPResponse> HTTPHandler::handleGET(const HTTPRequest& request) {
    std::string uri = request.getUri();
    std::string filePath = _root_directory + uri;
    if (_cgiHandler.canHandle(filePath)) {
        auto cgiResponse = _cgiHandler.executeCGI(uri, request);
        if (cgiResponse) {
            return cgiResponse;
        } else {
            return handleError(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
        }
    } else {
        return serveResource(uri);
    }
}
std::unique_ptr<HTTPResponse> HTTPHandler::handlePOST(const HTTPRequest& request) {
    (void)request;
    return handleError(HTTP::StatusCode::NOT_IMPLEMENTED);
}
std::unique_ptr<HTTPResponse> HTTPHandler::handleDELETE(const HTTPRequest& request) {
    (void)request;
    return handleError(HTTP::StatusCode::NOT_IMPLEMENTED);
}
std::unique_ptr<HTTPResponse> HTTPHandler::handleError(HTTP::StatusCode status) {
    return generateErrorResponse(status);
}

std::unique_ptr<HTTPResponse> HTTPHandler::generateErrorResponse(HTTP::StatusCode status) {
    auto response = std::make_unique<HTTPResponse>();
    response->setStatus(status);
    response->setContentType("text/html");
    response->setHeader("Connection", "close");
    
    std::string errorContent;
    auto errorPageIt = _custom_error_pages.find(status);
    if (errorPageIt != _custom_error_pages.end()) {
        std::string errorFilePath = _root_directory + errorPageIt->second;
        std::ifstream errorFile(errorFilePath);
        if (errorFile.is_open()) {
            std::stringstream buffer;
            buffer << errorFile.rdbuf();
            errorContent = buffer.str();
        } else {
            errorContent = getGenericErrorMessage(status);
        }
    } else {
        errorContent = getGenericErrorMessage(status);
    }
    
    response->setBody(errorContent);
    return response;
}

std::string HTTPHandler::getGenericErrorMessage(HTTP::StatusCode status) const {
    std::string statusStr = std::to_string(static_cast<int>(status));
    std::string statusText = HTTP::statusToString(status);
    return "<html><body><h1>" + statusStr + " " + statusText + "</h1></body></html>";
}
void HTTPHandler::setRootDirectory(const std::string& root) {
    _root_directory = root;
    _cgiHandler.setRootDirectory(root);
}
std::unique_ptr<HTTPResponse> HTTPHandler::serveResource(const std::string& uri) {
    auto response = std::make_unique<HTTPResponse>();
    std::string normalizedUri = uri;
    if (normalizedUri == "/") {
        normalizedUri = "/index.html";
    }
    std::string filePath = _root_directory + normalizedUri;
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (file.is_open()) {
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(size);
        if (file.read(buffer.data(), size)) {
            response->setStatus(HTTP::StatusCode::OK);
            response->setContentType(getMimeType(filePath));
            response->setBody(std::string(buffer.data(), size));
        } else {
            response->setStatus(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
            response->setContentType("text/html");
            response->setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
        }
    } else {
        response->setStatus(HTTP::StatusCode::NOT_FOUND);
        response->setContentType("text/html");
        response->setBody("<html><body><h1>404 Not Found</h1></body></html>");
    }
    response->setHeader("Connection", "close");
    return response;
}
