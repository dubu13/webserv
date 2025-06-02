#include "HTTP/core/HttpResponse.hpp"
#include "utils/Logger.hpp"
#include "utils/Utils.hpp"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <map>

std::string HttpResponse::formatDate() {
    auto now = std::time(nullptr);
    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&now), "%a, %d %b %Y %H:%M:%S GMT");
    return ss.str();
}

std::string HttpResponse::getDefaultStatusText(int code) {
    static const std::map<int, std::string> statusTexts = {
        {200, "OK"}, {201, "Created"}, {204, "No Content"},
        {301, "Moved Permanently"}, {302, "Found"}, {400, "Bad Request"},
        {403, "Forbidden"}, {404, "Not Found"}, {405, "Method Not Allowed"},
        {500, "Internal Server Error"}
    };

    if (auto it = statusTexts.find(code); it != statusTexts.end()) {
        return it->second;
    }
    return "Unknown";
}

std::string HttpResponse::str() const {
    std::ostringstream response;

    response << "HTTP/1.1 " << _statusCode << " " << _statusText << "\r\n";

    if (_headers.find("Date") == _headers.end()) {
        response << "Date: " << formatDate() << "\r\n";
    }
    if (_headers.find("Server") == _headers.end()) {
        response << "Server: webserv/1.0\r\n";
    }
    if (!_body.empty() && _headers.find("Content-Length") == _headers.end()) {
        response << "Content-Length: " << _body.length() << "\r\n";
    }
    if (_headers.find("Connection") == _headers.end()) {
        response << "Connection: close\r\n";
    }

    for (const auto& [name, value] : _headers) {
        if (name != "Content-Length") {
            response << name << ": " << value << "\r\n";
        }
    }

    response << "\r\n" << _body;
    return response.str();
}

HttpResponse& HttpResponse::status(int code, std::string_view text) {
    _statusCode = code;
    _statusText = text.empty() ? getDefaultStatusText(code) : std::string(text);
    return *this;
}

HttpResponse& HttpResponse::header(std::string_view name, std::string_view value) {
    _headers[std::string(name)] = value;
    return *this;
}

HttpResponse& HttpResponse::body(std::string_view content, std::string_view contentType) {
    _body = content;
    header("Content-Type", contentType);
    if (!content.empty()) {
        header("Content-Length", std::to_string(content.length()));
    }
    return *this;
}

void HttpResponse::setHeader(const std::pair<std::string_view, std::string_view>& header) {
    _headers[std::string(header.first)] = header.second;
}

std::string HttpResponse::ok(std::string_view content, std::string_view contentType) {
    return buildResponse(200, "OK", content, contentType);
}

std::string HttpResponse::notFound(std::string_view message) {
    return buildResponse(404, "Not Found", message, "text/plain");
}

std::string HttpResponse::badRequest(std::string_view message) {
    return buildResponse(400, "Bad Request", message, "text/plain");
}

std::string HttpResponse::methodNotAllowed(std::string_view message) {
    return buildResponse(405, "Method Not Allowed", message, "text/plain");
}

std::string HttpResponse::internalError(std::string_view message) {
    return buildResponse(500, "Internal Server Error", message, "text/plain");
}

std::string HttpResponse::buildResponse(int statusCode,
                                      const std::string& statusText,
                                      std::string_view content,
                                      std::string_view contentType) {
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << content;
    return response.str();
}

HttpResponse HttpResponse::redirect(std::string_view location, int code) {
    return HttpResponse().status(code, code == 301 ? "Moved Permanently" : "Found")
        .header("Location", location)
        .header("Content-Length", "0");
}

HttpResponse HttpResponse::file(std::string_view filePath) {
    try {
        HTTP::StatusCode status = HTTP::StatusCode::OK;
        std::string content = FileUtils::readFile("", filePath, status);

        if (status != HTTP::StatusCode::OK) {
            throw std::runtime_error("Failed to read file: " + std::to_string(static_cast<int>(status)));
        }

        std::string contentType = FileUtils::getMimeType(filePath);

        return HttpResponse().status(200, "OK").body(content, contentType);
    } catch (const std::exception& e) {
        return HttpResponse().status(404, "Not Found")
            .body(
                "<!DOCTYPE html><html><head><title>404 Not Found</title></head>"
                "<body><h1>404 Not Found</h1><p>" + std::string(e.what()) + "</p></body></html>",
                "text/html"
            );
    }
}

HttpResponse HttpResponse::directory(std::string_view dirPath, std::string_view uri) {

    std::string dirPathStr(dirPath);
    std::string uriStr(uri);
    return HttpResponse().status(200, "OK").body(FileUtils::generateDirectoryListing(dirPathStr, uriStr), "text/html");
}
