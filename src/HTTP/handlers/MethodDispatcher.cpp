#include "HTTP/handlers/MethodDispatcher.hpp"
#include "HTTP/core/ErrorResponseBuilder.hpp"
#include "HTTP/core/HttpResponse.hpp"
#include "HTTP/handlers/StaticFileHandler.hpp"
#include "resource/CGIHandler.hpp"
#include "utils/Logger.hpp"
#include "utils/Utils.hpp"
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>

using HTTP::Method;
using HTTP::methodToString;
using HTTP::Request;
using HTTP::StatusCode;

std::string MethodHandler::handleRequest(const Request &request,
                                         std::string_view root,
                                         const RequestRouter *router) {

  if (router) {
    const LocationBlock *location = router->findLocation(request.requestLine.uri);
    if (location) {
      if (!router->isMethodAllowed(request, location)) {
        return ErrorResponseBuilder::buildResponse(405);
      }
      if (router->hasRedirection(location)) {
        return router->handleRedirection(location);
      }
      std::string resolvedRoot = router->resolveRoot(location);
      root = resolvedRoot;
    }
  }

  switch (request.requestLine.method) {
  case Method::GET:
    return handleGet(request, root, router);
  case Method::POST:
    return handlePost(request, root, router);
  case Method::DELETE:
    return handleDelete(request, root, router);
  default:
    Logger::logf<LogLevel::WARN>("Unsupported method: %s",
                                methodToString(request.requestLine.method).c_str());
    return ErrorResponseBuilder::buildResponse(405);
  }
}

std::pair<std::string, std::string>
MethodHandler::resolvePaths(const Request &request, std::string_view root,
                            const RequestRouter *router) {
  std::string effectiveRoot = HttpUtils::getEffectiveRoot(root);
  std::string cleanUri = HttpUtils::cleanUri(request.requestLine.uri);
  std::string uriToUse = cleanUri;

  if (router) {
    const LocationBlock *location = router->findLocation(cleanUri);
    uriToUse = router->getRelativePath(cleanUri, location);
  }

  std::string filePath = HttpUtils::buildPath(effectiveRoot, uriToUse);
  return {effectiveRoot, filePath};
}

std::string
MethodHandler::handleGet(const Request &request, std::string_view root,
                         const RequestRouter *router) {
  auto [effectiveRoot, filePath] = resolvePaths(request, root, router);

  CGIHandler cgiHandler(effectiveRoot);
  if (cgiHandler.canHandle(filePath)) {
    std::string relativeUri = HttpUtils::cleanUri(request.requestLine.uri);
    if (router) {
      const LocationBlock *location = router->findLocation(request.requestLine.uri);
      relativeUri = router->getRelativePath(request.requestLine.uri, location);
    }
    return cgiHandler.executeCGI(relativeUri, request);
  }
  std::string relativeUri = HttpUtils::cleanUri(request.requestLine.uri);
  if (router) {
    const LocationBlock *location = router->findLocation(request.requestLine.uri);
    relativeUri = router->getRelativePath(request.requestLine.uri, location);
  }
  return StaticFileHandler::handleRequest(effectiveRoot, request.requestLine.uri, router);
}

std::string
MethodHandler::handlePost(const Request &request, std::string_view root,
                          const RequestRouter *router) {
  auto [effectiveRoot, filePath] = resolvePaths(request, root, router);

  auto contentTypeIt = request.headers.find("Content-Type");
  if (contentTypeIt != request.headers.end()) {
    std::string_view contentType = contentTypeIt->second;

    if (contentType.find("multipart/form-data") != std::string_view::npos)
      return handleFileUpload(request, effectiveRoot, contentType, router);
    CGIHandler cgiHandler(effectiveRoot);
    if (cgiHandler.canHandle(filePath)) {
      std::string relativeUri = HttpUtils::cleanUri(request.requestLine.uri);
      if (router) {
        const LocationBlock *location = router->findLocation(request.requestLine.uri);
        relativeUri = router->getRelativePath(request.requestLine.uri, location);
      }
      return cgiHandler.executeCGI(relativeUri, request);
    }
  }
  CGIHandler cgiHandler(effectiveRoot);
  if (cgiHandler.canHandle(filePath)) {
    std::string relativeUri = HttpUtils::cleanUri(request.requestLine.uri);
    if (router) {
      const LocationBlock *location = router->findLocation(request.requestLine.uri);
      relativeUri = router->getRelativePath(request.requestLine.uri, location);
    }
    return cgiHandler.executeCGI(relativeUri, request);
  }
  Logger::logf<LogLevel::INFO>("POST request to static resource: %s", filePath.c_str());
  return HttpResponse::ok("POST request processed successfully", "text/plain");
}

std::string
MethodHandler::handleFileUpload(const Request &request, const std::string &effectiveRoot,
                               std::string_view contentType, const RequestRouter *router) {

  std::string uploadPath;
  if (router) {
    const LocationBlock *location = router->findLocation(request.requestLine.uri);
    if (location && location->uploadEnable && !location->uploadStore.empty())
      uploadPath = location->uploadStore;
  }
  if (uploadPath.empty())
    uploadPath = HttpUtils::buildPath(effectiveRoot, "uploads");
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
  for (const auto &file : files) {
    std::string filename = file.filename.empty()
                         ? "upload_" + std::to_string(std::time(nullptr))
                         : file.filename;
    std::string fullPath = HttpUtils::buildPath(uploadPath, filename);

    if (!uploadedFiles.empty()) uploadedFiles += ", ";
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
    Logger::logf<LogLevel::INFO>("File uploaded successfully: %s (%zu bytes)",
                                fullPath.c_str(), file.content.size());
  }
  return HttpResponse::ok("Files uploaded successfully: " + uploadedFiles, "text/plain");
}

std::string
MethodHandler::handleDelete(const Request &request, std::string_view root,
                            const RequestRouter *router) {
  auto [effectiveRoot, filePath] = resolvePaths(request, root, router);

  std::string relativeUri = HttpUtils::cleanUri(request.requestLine.uri);
  if (router) {
    const LocationBlock *location = router->findLocation(request.requestLine.uri);
    relativeUri = router->getRelativePath(request.requestLine.uri, location);
  }

  StatusCode status;
  bool deleteResult = FileUtils::deleteFile(effectiveRoot, relativeUri, status);
  
  if (!deleteResult)
    return ErrorResponseBuilder::buildResponse(static_cast<int>(status));
  
  Logger::logf<LogLevel::INFO>("File deleted successfully: %s", filePath.c_str());
  return HttpResponse::ok("File deleted successfully", "text/plain");
}
