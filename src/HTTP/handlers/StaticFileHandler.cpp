#include "HTTP/handlers/StaticFileHandler.hpp"
#include "HTTP/core/HttpResponse.hpp"
#include "utils/Utils.hpp"
#include "utils/Logger.hpp"
#include <filesystem>

using HTTP::StatusCode;

std::string StaticFileHandler::handleRequest(std::string_view root, std::string_view uri) {
    Logger::debugf("=== StaticFileHandler::handleRequest START ===");
    Logger::debugf("Root: '%.*s', URI: '%.*s'",
                 static_cast<int>(root.length()), root.data(),
                 static_cast<int>(uri.length()), uri.data());

    std::string effectiveRoot = root.empty() ? "./www" : std::string(root);
    std::string normalizedUri = uri.empty() ? "/" : std::string(uri);
    std::string filePath = HttpUtils::buildPath(effectiveRoot, normalizedUri);

    Logger::debugf("Resolved file path: '%s'", filePath.c_str());

    if (!HttpUtils::isPathSafe(normalizedUri)) {
        Logger::warnf("Unsafe path detected: %s", normalizedUri.c_str());
        return HttpResponse::badRequest("Invalid path");
    }

    if (!std::filesystem::exists(filePath)) {
        Logger::debugf("Path does not exist: %s", filePath.c_str());
        return HttpResponse::notFound("File not found");
    }

    if (std::filesystem::is_directory(filePath)) {
        return serveDirectory(filePath, normalizedUri);
    } else {
        return serveFile(filePath);
    }
}

std::string StaticFileHandler::serveFile(std::string_view filePath) {
    Logger::debugf("Serving file: %.*s", static_cast<int>(filePath.length()), filePath.data());

    StatusCode status;
    std::string content = FileUtils::readFile("", filePath, status);

    if (status != StatusCode::OK) {
        Logger::debugf("Failed to read file: %.*s", static_cast<int>(filePath.length()), filePath.data());
        return HttpResponse::notFound("Failed to read file");
    }

    std::string contentType = FileUtils::getMimeType(filePath);
    Logger::debugf("Serving %zu bytes as %s", content.length(), contentType.c_str());

    return HttpResponse::ok(content, contentType);
}

std::string StaticFileHandler::serveDirectory(std::string_view dirPath, std::string_view requestUri) {
    Logger::debugf("Serving directory: %.*s", static_cast<int>(dirPath.length()), dirPath.data());

    std::string indexPath = findIndexFile(dirPath);
    if (!indexPath.empty()) {
        Logger::debugf("Found index file: %s", indexPath.c_str());
        return serveFile(indexPath);
    }

    Logger::debugf("No index file found in directory");

    if (requestUri == "/") {
        return generateWelcomePage();
    }

    return HttpResponse::notFound("Directory listing not available");
}

std::string StaticFileHandler::findIndexFile(std::string_view dirPath) {
    const char* indexFiles[] = {"index.html", "index.htm", nullptr};

    for (const char** indexFile = indexFiles; *indexFile; ++indexFile) {
        std::string indexPath = std::string(dirPath) + "/" + *indexFile;
        if (std::filesystem::exists(indexPath) && !std::filesystem::is_directory(indexPath)) {
            return indexPath;
        }
    }

    return "";
}

std::string StaticFileHandler::generateWelcomePage() {
    constexpr std::string_view welcomePage =
        "<!DOCTYPE html><html><head><title>Welcome to WebServ</title>"
        "<style>body{font-family:sans-serif;text-align:center;margin-top:20%;}"
        "h1{color:#2c3e50;}p{color:#7f8c8d;}</style></head>"
        "<body><h1>Welcome to WebServ</h1>"
        "<p>Your server is running correctly, but no index file was found.</p>"
        "<p>Please create an index.html file in your root directory.</p></body></html>";

    return HttpResponse::ok(welcomePage, "text/html");
}
