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

bool parseRequest(const std::string& data, Request& request) {
    Logger::warnf("Parsing request data (first 200 chars): %s", data.substr(0, 200).c_str());

    std::istringstream stream(data);
    std::string requestLine;

    if (!std::getline(stream, requestLine)) {
        Logger::error("Failed to read request line");
        return false;
    }

    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    Logger::warnf("Request line: '%s'", requestLine.c_str());

    std::istringstream lineStream(requestLine);
    std::string methodStr, uri, version;

    if (!(lineStream >> methodStr >> uri >> version)) {
        Logger::error("Failed to parse request line components");
        return false;
    }

    Logger::warnf("Parsed - Method: '%s', URI: '%s', Version: '%s'",
                   methodStr.c_str(), uri.c_str(), version.c_str());

    request.requestLine.method = stringToMethod(methodStr);
    if (request.requestLine.method == Method::UNKNOWN) {
        Logger::error("Invalid HTTP method: " + methodStr);
        return false;
    }

    if (uri.empty() || uri[0] != '/') {
        Logger::error("Invalid URI: must start with /");
        return false;
    }

    if (!HttpUtils::validateUriLength(uri.length())) {
        return false;
    }

    if (!HttpUtils::isPathSafe(uri)) {
        return false;
    }

    if (version != "HTTP/1.1" && version != "HTTP/1.0") {
        Logger::error("Invalid HTTP version: " + version);
        return false;
    }

    request.requestLine.uri = uri;
    request.requestLine.version = version;

    std::string headerLine;
    while (std::getline(stream, headerLine) && !headerLine.empty() && headerLine != "\r") {
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }

        size_t colonPos = headerLine.find(':');
        if (colonPos != std::string::npos) {
            std::string name = headerLine.substr(0, colonPos);
            std::string value = headerLine.substr(colonPos + 1);

            name = HttpUtils::trimWhitespace(name);
            value = HttpUtils::trimWhitespace(value);

            request.headers[name] = value;
        }
    }

    if (request.requestLine.version == "HTTP/1.1") {
        auto hostIt = request.headers.find("Host");
        if (hostIt == request.headers.end() || hostIt->second.empty()) {
            Logger::error("Missing required Host header for HTTP/1.1");
            return false;
        }
    }

    auto contentLengthIt = request.headers.find("Content-Length");
    if (contentLengthIt != request.headers.end()) {
        size_t contentLength;
        if (!HttpUtils::validateContentLength(contentLengthIt->second, contentLength)) {
            Logger::error("Invalid Content-Length value");
            return false;
        }
    }

    return true;
}

bool parseRequestLine(std::string_view line, RequestLine& requestLine) {
    std::istringstream stream{std::string{line}};
    std::string method;

    if (!(stream >> method >> requestLine.uri >> requestLine.version)) {
        Logger::error("Invalid request line format");
        return false;
    }

    if (method == "GET") {
        requestLine.method = Method::GET;
    } else if (method == "POST") {
        requestLine.method = Method::POST;
    } else if (method == "DELETE") {
        requestLine.method = Method::DELETE;
    } else {
        Logger::error("Invalid HTTP method");
        return false;
    }

    return true;
}

bool parseHeaders(std::string_view headerSection, std::map<std::string, std::string>& headers) {
    std::istringstream stream{std::string{headerSection}};
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty() || line == "\r") {
            continue;
        }

        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            Logger::error("Invalid header format");
            return false;
        }

        std::string name = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);

        value = std::string(HttpUtils::trimWhitespace(value));

        headers[name] = value;
    }

    return true;
}

bool parseChunkedBody(std::string_view data, size_t bodyStart, std::string& body) {
    body.clear();
    size_t pos = bodyStart;
    size_t chunkCount = 0;

    while (pos < data.length()) {
        if (!HttpUtils::validateChunkCount(++chunkCount)) {
            return false;
        }

        size_t chunkSize;
        if (!HttpUtils::parseChunkSize(data, pos, chunkSize)) {
            return false;
        }

        if (pos + chunkSize + 2 > data.length()) {
            Logger::error("HTTP/1.1 Error: Incomplete chunk data");
            return false;
        }

        if (chunkSize > 0) {
            body.append(data.substr(pos, chunkSize));
            pos += chunkSize;
        }

        if (!HttpUtils::validateChunkTerminator(data, pos)) {
            return false;
        }
        pos += 2;

        if (!HttpUtils::validateBodySize(body.length())) {
            return false;
        }

        if (chunkSize == 0) {
            return true;
        }
    }

    Logger::error("HTTP/1.1 Error: Malformed chunked body");
    return false;
}

bool validateHttpRequest(const Request& request) {
    if (request.requestLine.version != "HTTP/1.1" &&
        request.requestLine.version != "HTTP/1.0") {
        Logger::error("HTTP/1.1 Error: Unsupported version " + request.requestLine.version);
        return false;
    }

    if (request.requestLine.version == "HTTP/1.1") {
        auto host_it = request.headers.find("Host");
        if (host_it == request.headers.end() || host_it->second.empty()) {
            Logger::error("HTTP/1.1 Error: Missing required Host header");
            return false;
        }
    }

    return true;
}

bool parseBody(std::string_view data, size_t bodyStart, std::string& body) {
    if (bodyStart < data.length()) {
        body = std::string(data.substr(bodyStart));
        return true;
    }
    body.clear();
    return true;
}

}
