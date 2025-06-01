#include "HTTP/HTTPResponseBuilder.hpp"
#include "HTTP/HTTPTypes.hpp"
#include <sstream>
#include <ctime>
#include <iomanip>

namespace HTTP {

ResponseBuilder::ResponseBuilder(HTTP::StatusCode status)
    : _status(status), _keepAlive(false), _chunked(false) {
    setStandardHeaders();
}

std::string ResponseBuilder::buildStatusLine() const {
    return "HTTP/1.1 " + std::to_string(static_cast<int>(_status)) + " " + 
           HTTP::statusToString(_status) + "\r\n";
}

std::string ResponseBuilder::buildHeaders() const {
    std::string headers;
    headers.reserve(_headers.size() * 50);
    
    for (const auto& header : _headers) {
        headers += header.first + ": " + header.second + "\r\n";
    }
    
    return headers;
}

void ResponseBuilder::setStandardHeaders() {
    // Set default server header
    _headers["Server"] = "webserv/1.0";
    
    // Set current date
    std::time_t now = std::time(nullptr);
    std::stringstream oss;
    oss << std::put_time(std::gmtime(&now), "%a, %d %b %Y %H:%M:%S GMT");
    _headers["Date"] = oss.str();
}

ResponseBuilder& ResponseBuilder::setStatus(HTTP::StatusCode status) {
    _status = status;
    return *this;
}

ResponseBuilder& ResponseBuilder::setHeader(const std::string& name, const std::string& value) {
    _headers[name] = value;
    return *this;
}

ResponseBuilder& ResponseBuilder::setContentType(const std::string& type) {
    _headers["Content-Type"] = type;
    return *this;
}

ResponseBuilder& ResponseBuilder::setBody(const std::string& body) {
    _body = body;
    if (!_chunked) {
        _headers["Content-Length"] = std::to_string(body.size());
    }
    return *this;
}

ResponseBuilder& ResponseBuilder::setKeepAlive(bool keepAlive) {
    _keepAlive = keepAlive;
    if (keepAlive) {
        _headers["Connection"] = "keep-alive";
        _headers["Keep-Alive"] = "timeout=5, max=100";
    } else {
        _headers["Connection"] = "close";
        _headers.erase("Keep-Alive");
    }
    return *this;
}

ResponseBuilder& ResponseBuilder::setChunked(bool chunked) {
    _chunked = chunked;
    if (chunked) {
        _headers["Transfer-Encoding"] = "chunked";
        _headers.erase("Content-Length");
    } else {
        _headers.erase("Transfer-Encoding");
        if (!_body.empty()) {
            _headers["Content-Length"] = std::to_string(_body.size());
        }
    }
    return *this;
}

ResponseBuilder& ResponseBuilder::setCacheControl(const std::string& control) {
    _headers["Cache-Control"] = control;
    return *this;
}

std::string ResponseBuilder::build() {
    std::string response;
    response.reserve(256 + _body.length() + _headers.size() * 50);
    
    response += buildStatusLine();
    response += buildHeaders();
    response += "\r\n";
    
    if (_chunked && !_body.empty()) {
        // Create chunked body
        std::stringstream chunkedBody;
        chunkedBody << std::hex << _body.size() << "\r\n";
        chunkedBody << _body << "\r\n";
        chunkedBody << "0\r\n\r\n";
        response += chunkedBody.str();
    } else {
        response += _body;
    }
    
    return response;
}

std::string ResponseBuilder::createSimpleResponse(HTTP::StatusCode status, const std::string& message) {
    return ResponseBuilder(status)
        .setContentType("text/plain")
        .setBody(message)
        .build();
}

std::string ResponseBuilder::createErrorResponse(HTTP::StatusCode status, const std::string& errorMessage) {
    std::string body = errorMessage;
    if (body.empty()) {
        body = "<!DOCTYPE html><html><head><title>Error " + std::to_string(static_cast<int>(status)) + 
               "</title></head><body><h1>Error " + std::to_string(static_cast<int>(status)) + ": " + 
               HTTP::statusToString(status) + "</h1></body></html>";
    }
    
    return ResponseBuilder(status)
        .setContentType("text/html")
        .setBody(body)
        .setCacheControl("no-cache")
        .build();
}

std::string ResponseBuilder::createFileResponse(HTTP::StatusCode status, const std::string& content, const std::string& contentType) {
    bool cacheable = (contentType.find("text/html") == std::string::npos);
    std::string cacheControl = cacheable ? "public, max-age=3600" : "no-cache";
    
    return ResponseBuilder(status)
        .setContentType(contentType)
        .setBody(content)
        .setCacheControl(cacheControl)
        .build();
}

std::string ResponseBuilder::createRedirectResponse(HTTP::StatusCode status, const std::string& location) {
    return ResponseBuilder(status)
        .setHeader("Location", location)
        .setContentType("text/html")
        .setBody("")
        .build();
}

std::string ResponseBuilder::createChunkedFileResponse(HTTP::StatusCode status, const std::string& content, const std::string& contentType) {
    return ResponseBuilder(status)
        .setContentType(contentType)
        .setBody(content)
        .setChunked(true)
        .setCacheControl("no-cache")
        .build();
}

} // namespace HTTP
