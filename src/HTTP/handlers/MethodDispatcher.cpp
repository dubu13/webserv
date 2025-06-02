#include "HTTP/handlers/MethodDispatcher.hpp"
#include "HTTP/handlers/StaticFileHandler.hpp"
#include "HTTP/core/HttpResponse.hpp"
#include "utils/Utils.hpp"
#include "utils/Logger.hpp"
#include "resource/CGIHandler.hpp"
#include <filesystem>
#include <fstream>
#include <chrono>

using HTTP::Request;
using HTTP::Method;
using HTTP::StatusCode;
using HTTP::methodToString;

std::string MethodHandler::handleRequest(const Request& request, std::string_view root) {
    Logger::debugf("Handling %s request for URI: %s",
                  methodToString(request.requestLine.method).c_str(),
                  request.requestLine.uri.c_str());

    switch (request.requestLine.method) {
        case Method::GET:
            return handleGet(request, root);
        case Method::POST:
            return handlePost(request, root);
        case Method::DELETE:
            return handleDelete(request, root);
        default:
            return HttpResponse::methodNotAllowed("Method not allowed");
    }
}

std::string MethodHandler::handleGet(const Request& request, std::string_view root) {
    std::string effectiveRoot = root.empty() ? "./www" : std::string(root);
    std::string uri = request.requestLine.uri;

    Logger::debugf("GET Handler - Root: '%s', URI: '%s'", effectiveRoot.c_str(), uri.c_str());

    std::string filePath = HttpUtils::buildPath(effectiveRoot, uri);
    Logger::debugf("Complete file path: '%s'", filePath.c_str());

    CGIHandler cgiHandler(effectiveRoot);
    if (cgiHandler.canHandle(filePath)) {
        Logger::debugf("Handling CGI request for: %s", filePath.c_str());
        return cgiHandler.executeCGI(uri, request);
    }

    return StaticFileHandler::handleRequest(effectiveRoot, uri);
}

std::string MethodHandler::handlePost(const Request& request, std::string_view root) {
    Logger::debugf("POST request for URI: %s", request.requestLine.uri.c_str());

    std::string effectiveRoot = root.empty() ? "./www" : std::string(root);
    std::string filePath = HttpUtils::buildPath(effectiveRoot, request.requestLine.uri);

    auto contentTypeIt = request.headers.find("Content-Type");
    if (contentTypeIt != request.headers.end()) {
        std::string_view contentType = contentTypeIt->second;

        if (contentType.find("multipart/form-data") != std::string_view::npos) {

            std::string uploadPath = HttpUtils::buildPath(effectiveRoot, "uploads");

            if (!FileUtils::exists(uploadPath)) {
                if (!FileUtils::createDirectories(uploadPath)) {
                    Logger::error("Failed to create upload directory");
                    return HttpResponse::internalError("Failed to create upload directory");
                }
            }

            std::string timestamp = "upload_" + std::to_string(std::time(nullptr));
            std::string fullPath = HttpUtils::buildPath(uploadPath, timestamp + ".dat");

            if (!FileUtils::writeFileContent(fullPath, request.body)) {
                Logger::errorf("Failed to write uploaded file: %s", fullPath.c_str());
                return HttpResponse::internalError("Failed to save uploaded file");
            }

            Logger::infof("File uploaded successfully: %s", fullPath.c_str());
            return HttpResponse::ok("File uploaded successfully", "text/plain");
        }
    }

    return HttpResponse::badRequest("Invalid POST request");
}

std::string MethodHandler::handleDelete(const Request& request, std::string_view root) {
    Logger::debugf("DELETE request for URI: %s", request.requestLine.uri.c_str());

    std::string effectiveRoot = root.empty() ? "./www" : std::string(root);
    std::string filePath = HttpUtils::buildPath(effectiveRoot, request.requestLine.uri);

    if (!FileUtils::exists(filePath)) {
        return HttpResponse::notFound("File not found");
    }

    StatusCode status;
    bool deleteResult = FileUtils::deleteFile("", filePath, status);
    if (!deleteResult) {
        return HttpResponse::internalError("Failed to delete file");
    }

    Logger::infof("File deleted successfully: %s", filePath.c_str());
    return HttpResponse::ok("File deleted successfully", "text/plain");
}
