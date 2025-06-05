#include <iostream>
#include <map>
#include <sstream>
#include <string_view>
#include <string>
#include <algorithm>

#include "HTTP/core/HTTPParser.hpp"
#include "HTTP/core/HTTPTypes.hpp"
#include "utils/Utils.hpp"
#include "Logger.hpp"

namespace HTTP {

// Consolidated validation and utility helpers
template<typename T>
static bool validate(T condition, const std::string& errorMsg) {
    if (!condition) Logger::error(errorMsg);
    return static_cast<bool>(condition);
}

static void cleanLineEnding(std::string& line) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
}

static std::string getHeader(const std::map<std::string, std::string>& headers, const std::string& name) {
    auto it = headers.find(name);
    return it != headers.end() ? it->second : "";
}

static bool isValidHttpVersion(const std::string& version) {
    return version == "HTTP/1.1" || version == "HTTP/1.0";
}

// Main parsing functions
bool parseRequest(const std::string& data, Request& request) {
    size_t headerEnd = data.find("\r\n\r\n");
    if (!validate(headerEnd != std::string::npos, "Malformed HTTP request: missing header-body separator")) return false;

    std::istringstream stream(data.substr(0, headerEnd));
    std::string requestLineStr;
    
    if (!std::getline(stream, requestLineStr)) {
        Logger::error("Failed to read request line");
        return false;
    }
    cleanLineEnding(requestLineStr);
    
    return parseRequestLine(requestLineStr, request.requestLine) &&
           parseHeaders(stream, request.headers) &&
           validateHttpRequest(request) &&
           parseContentLength(request) &&
           parseRequestBody(data, headerEnd + 4, request);
}

bool parseRequestLine(std::string_view line, RequestLine& requestLine) {
    std::istringstream stream{std::string{line}};
    std::string methodStr;

    if (!(stream >> methodStr >> requestLine.uri >> requestLine.version)) {
        Logger::error("Invalid request line format");
        return false;
    }

    requestLine.method = stringToMethod(methodStr);
    return validate(requestLine.method != Method::UNKNOWN, "Invalid HTTP method: " + methodStr) &&
           validate(!requestLine.uri.empty() && requestLine.uri[0] == '/', "Invalid URI: must start with /") &&
           validate(HttpUtils::validateUriLength(requestLine.uri.length()), "URI too long") &&
           validate(HttpUtils::isPathSafe(requestLine.uri), "Unsafe URI path") &&
           validate(isValidHttpVersion(requestLine.version), "Invalid HTTP version: " + requestLine.version);
}

bool parseHeaders(std::istringstream& stream, std::map<std::string, std::string>& headers) {
    std::string line;
    while (std::getline(stream, line) && !line.empty()) {
        cleanLineEnding(line);
        if (!parseHeader(line, headers)) return false;
    }
    return true;
}

bool parseHeaders(std::string_view headerSection, std::map<std::string, std::string>& headers) {
    std::istringstream stream{std::string{headerSection}};
    return parseHeaders(stream, headers);
}

bool parseHeader(std::string_view line, std::map<std::string, std::string>& headers) {
    size_t colonPos = line.find(':');
    if (!validate(colonPos != std::string::npos, "Invalid header format")) return false;
    
    headers[std::string(HttpUtils::trimWhitespace(line.substr(0, colonPos)))] = 
        std::string(HttpUtils::trimWhitespace(line.substr(colonPos + 1)));
    return true;
}

bool parseContentLength(Request& request) {
    std::string contentLength = getHeader(request.headers, "Content-Length");
    return contentLength.empty() || 
           validate(HttpUtils::validateContentLength(contentLength, request.contentLength), "Invalid Content-Length");
}

bool validateHttpRequest(const Request& request) {
    return validate(isValidHttpVersion(request.requestLine.version), "Unsupported version " + request.requestLine.version) &&
           (request.requestLine.version != "HTTP/1.1" || validate(!getHeader(request.headers, "Host").empty(), "Missing Host header for HTTP/1.1"));
}

bool parseRequestBody(const std::string& data, size_t bodyStart, Request& request) {
    std::string transferEncoding = getHeader(request.headers, "Transfer-Encoding");
    request.chunkedTransfer = transferEncoding.find("chunked") != std::string::npos;

    bool success = request.chunkedTransfer ? 
        parseChunkedBody(data, bodyStart, request.body) : 
        parseBody(data, bodyStart, request.body);

    if (!validate(success, request.chunkedTransfer ? "Failed to parse chunked body" : "Failed to parse request body")) 
        return false;

    // Validate body size matches Content-Length if specified
    std::string contentLength = getHeader(request.headers, "Content-Length");
    if (!contentLength.empty() && request.body.size() != request.contentLength) {
        Logger::warnf("Body size (%zu) doesn't match Content-Length (%zu)", 
                     request.body.size(), request.contentLength);
    }
    return true;
}

bool parseChunkedBody(std::string_view data, size_t bodyStart, std::string& body) {
    body.clear();
    size_t pos = bodyStart, chunkCount = 0;

    while (pos < data.length()) {
        size_t chunkSize;
        if (!HttpUtils::validateChunkCount(++chunkCount) ||
            !HttpUtils::parseChunkSize(data, pos, chunkSize) ||
            pos + chunkSize + 2 > data.length()) {
            return validate(false, "HTTP/1.1 Error: Incomplete chunk data");
        }

        if (chunkSize == 0) return true; // End of chunks
        
        body.append(data.substr(pos, chunkSize));
        pos += chunkSize;

        if (!HttpUtils::validateChunkTerminator(data, pos) || 
            !HttpUtils::validateBodySize(body.length())) {
            return validate(false, chunkSize == 0 ? "HTTP/1.1 Error: Malformed chunked body" : "Body size too large");
        }
        pos += 2;
    }

    return validate(false, "HTTP/1.1 Error: Malformed chunked body");
}

bool parseBody(std::string_view data, size_t bodyStart, std::string& body) {
    body = bodyStart < data.length() ? std::string(data.substr(bodyStart)) : "";
    return true;
}

// Boundary extraction utility
static std::string extractBoundary(std::string_view contentType) {
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string_view::npos) return "";
    
    boundaryPos += 9; // Skip "boundary="
    size_t boundaryEnd = contentType.find(";", boundaryPos);
    std::string boundary = std::string(contentType.substr(boundaryPos, 
        boundaryEnd == std::string_view::npos ? contentType.length() - boundaryPos : boundaryEnd - boundaryPos));
    
    // Remove quotes if present
    if (boundary.length() >= 2 && boundary.front() == '"' && boundary.back() == '"') {
        boundary = boundary.substr(1, boundary.length() - 2);
    }
    return boundary;
}

// Multipart parsing
std::vector<MultipartFile> parseMultipartData(const std::string& body, std::string_view contentType) {
    std::vector<MultipartFile> files;
    std::string boundary = extractBoundary(contentType);
    
    if (!validate(!boundary.empty(), "No boundary found in multipart content-type")) return files;
    
    std::string boundaryMarker = "--" + boundary;
    size_t pos = body.find(boundaryMarker);
    
    while (pos != std::string::npos) {
        pos += boundaryMarker.length();
        
        // Check for end marker
        if (pos + 1 < body.length() && body.substr(pos, 2) == "--") break;
        
        // Skip to content (find double CRLF)
        pos = body.find("\r\n\r\n", pos);
        if (pos == std::string::npos) break;
        pos += 4;
        
        // Find next boundary
        size_t nextBoundary = body.find("\r\n" + boundaryMarker, pos);
        if (nextBoundary == std::string::npos) break;
        
        // Extract filename from headers within reasonable range
        MultipartFile file;
        size_t headerStart = body.rfind("filename=\"", pos);
        if (headerStart != std::string::npos && headerStart > pos - 200) {
            headerStart += 10; // Skip 'filename="'
            size_t headerEnd = body.find("\"", headerStart);
            if (headerEnd != std::string::npos) {
                file.filename = body.substr(headerStart, headerEnd - headerStart);
            }
        }
        
        // Extract content
        file.content = body.substr(pos, nextBoundary - pos);
        if (!file.content.empty()) files.push_back(std::move(file));
        
        pos = nextBoundary;
    }
    return files;
}

}
