#include "HTTP/handlers/StaticFileHandler.hpp"
#include "HTTP/core/HttpResponse.hpp"
#include "HTTP/core/ErrorResponseBuilder.hpp"
#include "utils/Utils.hpp"
#include "utils/ValidationUtils.hpp"
#include "utils/Logger.hpp"
#include <filesystem>

using HTTP::StatusCode;

std::string StaticFileHandler::handleRequest(std::string_view root, std::string_view uri) {

    std::string effectiveRoot = HttpUtils::getEffectiveRoot(root);
    std::string normalizedUri = uri.empty() ? "/" : std::string(uri);
    std::string filePath = HttpUtils::buildPath(effectiveRoot, normalizedUri);

    if (!ValidationUtils::isPathSafe(normalizedUri)) {
        Logger::logf<LogLevel::WARN>("Unsafe path detected: %s", normalizedUri.c_str());
        return ErrorResponseBuilder::buildResponse(400);
    }

    if (!FileUtils::exists(filePath)) {
        return ErrorResponseBuilder::buildResponse(404);
    }

    if (std::filesystem::is_directory(filePath)) {
        return serveDirectory(filePath, normalizedUri);
    } else {
        return serveFile(filePath);
    }
}

std::string StaticFileHandler::serveFile(std::string_view filePath) {

    StatusCode status;
    std::string content = FileUtils::readFile("", filePath, status);

    if (status != StatusCode::OK) {
        return ErrorResponseBuilder::buildResponse(404);
    }

    std::string contentType = FileUtils::getMimeType(filePath);

    return HttpResponse::ok(content, contentType);
}

std::string StaticFileHandler::serveDirectory(std::string_view dirPath, std::string_view requestUri) {

    std::string indexPath = findIndexFile(dirPath);
    if (!indexPath.empty()) {
        return serveFile(indexPath);
    }

    if (requestUri == "/") {
        return generateWelcomePage();
    }

    return ErrorResponseBuilder::buildResponse(404);
}

std::string StaticFileHandler::findIndexFile(std::string_view dirPath) {
    const char* indexFiles[] = {"index.html", "index.htm", nullptr};

    for (const char** indexFile = indexFiles; *indexFile; ++indexFile) {
        std::string indexPath = std::string(dirPath) + "/" + *indexFile;
        if (FileUtils::exists(indexPath) && !std::filesystem::is_directory(indexPath)) {
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
