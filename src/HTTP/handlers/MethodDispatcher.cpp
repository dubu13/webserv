#include "HTTP/handlers/MethodDispatcher.hpp"
#include "HTTP/handlers/StaticFileHandler.hpp"
#include "HTTP/core/HttpResponse.hpp"
#include "HTTP/core/ErrorResponseBuilder.hpp"
#include "utils/Utils.hpp"
#include "utils/Logger.hpp"
#include "resource/CGIHandler.hpp"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <ctime>

using HTTP::Request;
using HTTP::Method;
using HTTP::StatusCode;
using HTTP::methodToString;

std::string MethodHandler::handleRequest(const Request& request, std::string_view root, const RequestRouter* router) {
    if (router) {
        const LocationBlock* location = router->findLocation(request.requestLine.uri);
        std::string redirectResponse = router->handleRedirection(location);
        if (!redirectResponse.empty())
            return redirectResponse;
        if (!router->isMethodAllowed(request, location))
            return ErrorResponseBuilder::buildResponse(405);
    }

    switch (request.requestLine.method) {
        case Method::GET:
            return handleGet(request, root, router);
        case Method::POST:
            return handlePost(request, root, router);
        case Method::DELETE:
            return handleDelete(request, root, router);
        default:
            return ErrorResponseBuilder::buildResponse(405);
    }
}

std::pair<std::string, std::string> MethodHandler::resolvePaths(const Request& request, std::string_view root) {
    std::string effectiveRoot = HttpUtils::getEffectiveRoot(root);
    std::string filePath = HttpUtils::buildPath(effectiveRoot, request.requestLine.uri);
    return {effectiveRoot, filePath};
}

std::string MethodHandler::handleGet(const Request& request, std::string_view root, [[maybe_unused]] const RequestRouter* router) {
    auto [effectiveRoot, filePath] = resolvePaths(request, root);
    
    CGIHandler cgiHandler(effectiveRoot);
    if (cgiHandler.canHandle(filePath)) {
        return cgiHandler.executeCGI(request.requestLine.uri, request);
    }

    return StaticFileHandler::handleRequest(effectiveRoot, request.requestLine.uri);
}

std::string MethodHandler::handlePost(const Request& request, std::string_view root, [[maybe_unused]] const RequestRouter* router) {
    auto [effectiveRoot, filePath] = resolvePaths(request, root);

    auto contentTypeIt = request.headers.find("Content-Type");
    if (contentTypeIt != request.headers.end()) {
        std::string_view contentType = contentTypeIt->second;

        if (contentType.find("multipart/form-data") != std::string_view::npos) {
            std::string uploadPath = HttpUtils::buildPath(effectiveRoot, "uploads");

            if (!FileUtils::exists(uploadPath)) {
                if (!FileUtils::createDirectories(uploadPath)) {
                    Logger::error("Failed to create upload directory");
                    return ErrorResponseBuilder::buildResponse(500);
                }
            }
            auto files = HTTP::parseMultipartData(request.body, contentType);
            if (files.empty()) {
                Logger::error("Failed to parse multipart form data");
                return ErrorResponseBuilder::buildResponse(400);
            }

            std::string uploadedFiles;
            for (const auto& file : files) {
                std::string filename = file.filename.empty() ?
                    "upload_" + std::to_string(std::time(nullptr)) : file.filename;
                std::string fullPath = HttpUtils::buildPath(uploadPath, filename);

                if (!uploadedFiles.empty()) {
                    uploadedFiles += ", ";
                }
                uploadedFiles += filename;

                std::ofstream outFile(fullPath, std::ios::binary);
                if (!outFile.is_open()) {
                    Logger::logf<LogLevel::ERROR>("Failed to open file for writing: %s", fullPath.c_str());
                    return ErrorResponseBuilder::buildResponse(500);
                }
                outFile.write(file.content.data(), file.content.size());
                outFile.close();
                if (!outFile.good()) {
                    Logger::logf<LogLevel::ERROR>("Failed to write uploaded file: %s", fullPath.c_str());
                    return ErrorResponseBuilder::buildResponse(500);
                }
                Logger::logf<LogLevel::INFO>("File uploaded successfully: %s (%zu bytes)", fullPath.c_str(), file.content.size());
            }
            return HttpResponse::ok("Files uploaded successfully: " + uploadedFiles, "text/plain");
        }
    }
    return ErrorResponseBuilder::buildResponse(400);
}

std::string MethodHandler::handleDelete(const Request& request, std::string_view root, [[maybe_unused]] const RequestRouter* router) {
    auto [effectiveRoot, filePath] = resolvePaths(request, root);

    if (!FileUtils::exists(filePath)) {
        return ErrorResponseBuilder::buildResponse(404);
    }

    StatusCode status;
    bool deleteResult = FileUtils::deleteFile("", filePath, status);
    if (!deleteResult) {
        return ErrorResponseBuilder::buildResponse(500);
    }

    Logger::logf<LogLevel::INFO>("File deleted successfully: %s", filePath.c_str());
    return HttpResponse::ok("File deleted successfully", "text/plain");
}
