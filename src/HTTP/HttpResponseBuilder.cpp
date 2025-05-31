#include "HTTP/HttpResponseBuilder.hpp"
#include "HTTP/HttpHeaderBuilder.hpp"
#include "HTTP/HTTPTypes.hpp"
#include "utils/Logger.hpp"

namespace HttpResponseBuilder {

std::string buildHttpStatusLine(int statusCode, const std::string& statusText, const std::string& version) {
    return version + " " + std::to_string(statusCode) + " " + statusText + "\r\n";
}

size_t estimateResponseSize(const std::string& version, 
                           const std::map<std::string, std::string>& headers,
                           const std::string& body, bool keepAlive) {
    size_t estimated = version.length() + 50 +
                      headers.size() * 50 +
                      body.length() + 20 +
                      (keepAlive ? 40 : 20);
    
    Logger::debugf("HttpResponseBuilder::estimateResponseSize - Estimated: %zu bytes", estimated);
    return estimated;
}

std::string buildResponse(HTTP::StatusCode status, const std::map<std::string, std::string>& headers, 
                         const std::string& body, bool keepAlive) {
    Logger::debugf("HttpResponseBuilder::buildResponse - Building response with status %d, %zu headers, %zu body bytes", 
                   static_cast<int>(status), headers.size(), body.size());
    
    std::string response;
    response.reserve(256 + body.length() + headers.size() * 50);
    
    // Status line
    response += "HTTP/1.1 " + std::to_string(static_cast<int>(status)) + " " + HTTP::statusToString(status) + "\r\n";
    
    // Connection headers
    if (keepAlive) {
        response += "Connection: keep-alive\r\nKeep-Alive: timeout=5, max=100\r\n";
    } else {
        response += "Connection: close\r\n";
    }
    
    // Other headers
    for (const auto& header : headers) {
        if (header.first != "Connection" && header.first != "Keep-Alive") {
            response += header.first + ": " + header.second + "\r\n";
            Logger::debugf("HTTP response header: %s: %s", header.first.c_str(), header.second.c_str());
        }
    }
    
    response += "\r\n" + body;
    Logger::debugf("HTTP response built successfully: %zu total bytes", response.size());
    return response;
}

std::string createSimpleResponse(HTTP::StatusCode status, const std::string& message) {
    Logger::debugf("HttpResponseBuilder::createSimpleResponse - Creating simple response with status %d", 
                   static_cast<int>(status));
    
    auto headers = HttpHeaderBuilder::createPlainTextHeaders(message.size());
    return buildResponse(status, headers, message);
}

std::string createErrorResponse(HTTP::StatusCode status, const std::string& errorMessage) {
    Logger::debugf("HttpResponseBuilder::createErrorResponse - Creating error response with status %d", 
                   static_cast<int>(status));
    
    std::string body = errorMessage;
    if (body.empty()) {
        body = "<!DOCTYPE html><html><head><title>Error " + std::to_string(static_cast<int>(status)) + 
               "</title></head><body><h1>Error " + std::to_string(static_cast<int>(status)) + ": " + 
               HTTP::statusToString(status) + "</h1></body></html>";
    }
    
    auto headers = HttpHeaderBuilder::createErrorHeaders(body.size());
    return buildResponse(status, headers, body);
}

std::string createFileResponse(HTTP::StatusCode status, const std::string& content, const std::string& contentType) {
    Logger::debugf("HttpResponseBuilder::createFileResponse - Creating file response with status %d, type %s", 
                   static_cast<int>(status), contentType.c_str());
    
    bool cacheable = (contentType.find("text/html") == std::string::npos);
    auto headers = HttpHeaderBuilder::createFileHeaders(contentType, content.size(), cacheable);
    return buildResponse(status, headers, content);
}

std::string createRedirectResponse(HTTP::StatusCode status, const std::string& location) {
    Logger::debugf("HttpResponseBuilder::createRedirectResponse - Creating redirect response with status %d to %s", 
                   static_cast<int>(status), location.c_str());
    
    std::map<std::string, std::string> headers;
    headers["Location"] = location;
    headers["Content-Type"] = "text/html";
    headers["Content-Length"] = "0";
    
    return buildResponse(status, headers, "");
}

}
