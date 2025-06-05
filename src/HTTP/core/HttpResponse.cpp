#include "HTTP/core/HttpResponse.hpp"
#include "HTTP/core/HTTPTypes.hpp"
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
    _statusText = text.empty() ? HTTP::statusToString(code) : std::string(text);
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

std::string HttpResponse::buildResponse(int statusCode,
                                      const std::string& statusText,
                                      std::string_view content,
                                      std::string_view contentType) {
    return HttpResponse()
        .status(statusCode, statusText)
        .body(content, contentType)
        .str();
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
        // Simple error page that avoids circular dependency
        std::string statusText = HTTP::statusToString(404);
        std::string errorContent = "<html><body><h1>404 " + statusText + "</h1></body></html>";
        return HttpResponse().status(404, statusText).body(errorContent, "text/html");
    }
}

HttpResponse HttpResponse::directory(std::string_view dirPath, std::string_view uri) {

    std::string dirPathStr(dirPath);
    std::string uriStr(uri);
    return HttpResponse().status(200, "OK").body(FileUtils::generateDirectoryListing(dirPathStr, uriStr), "text/html");
}
