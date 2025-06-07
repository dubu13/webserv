#include "HTTP/handlers/StaticFileHandler.hpp"
#include "HTTP/core/ErrorResponseBuilder.hpp"
#include "HTTP/core/HttpResponse.hpp"
#include "HTTP/routing/RequestRouter.hpp"
#include "utils/Logger.hpp"
#include "utils/Utils.hpp"
#include "utils/ValidationUtils.hpp"
#include <filesystem>

using HTTP::StatusCode;

std::string StaticFileHandler::handleRequest(std::string_view root,
                                             std::string_view uri,
                                             const RequestRouter *router) {

  std::string effectiveRoot = HttpUtils::getEffectiveRoot(root);
  std::string cleanUri = HttpUtils::cleanUri(uri);
  std::string originalUri = cleanUri;
  std::string relativeUri = cleanUri;
  if (router) {
    const LocationBlock *location = router->findLocation(cleanUri);
    relativeUri = router->getRelativePath(cleanUri, location);
  }

  std::string normalizedUri = relativeUri.empty() ? "/" : relativeUri;
  if (normalizedUri.find("../") != std::string::npos ||
      normalizedUri.find("..\\") != std::string::npos ||
      normalizedUri.find("%2e%2e%2f") != std::string::npos ||
      normalizedUri.find("%2e%2e/") != std::string::npos ||
      normalizedUri.find("..%2f") != std::string::npos)
    return ErrorResponseBuilder::buildResponse(403);

  std::string filePath = HttpUtils::buildPath(effectiveRoot, normalizedUri);

  if (!ValidationUtils::isPathSafe(normalizedUri))
    return ErrorResponseBuilder::buildResponse(403);
  if (!FileUtils::exists(filePath))
    return ErrorResponseBuilder::buildResponse(404);
  if (std::filesystem::is_directory(filePath))
    return serveDirectory(filePath, originalUri, router);
  return serveFile(filePath);
}

std::string StaticFileHandler::serveFile(std::string_view filePath) {
  StatusCode status;
  std::string content = FileUtils::readFile("", filePath, status);
  if (status != StatusCode::OK)
    return ErrorResponseBuilder::buildResponse(static_cast<int>(status));
  std::string contentType = FileUtils::getMimeType(filePath);
  return HttpResponse::ok(content, contentType);
}

std::string StaticFileHandler::serveDirectory(std::string_view dirPath,
                                              std::string_view requestUri,
                                              const RequestRouter *router) {

  std::string indexPath = findIndexFile(dirPath, requestUri, router);

  if (!indexPath.empty() && FileUtils::exists(indexPath) && !std::filesystem::is_directory(indexPath))
    return serveFile(indexPath);
  if (router) {
    const LocationBlock *location = router->findLocation(std::string(requestUri));
    if (location->autoindex)
      return HttpResponse::directory(dirPath, requestUri);
  }
  return ErrorResponseBuilder::buildResponse(404);
}

std::string StaticFileHandler::findIndexFile(std::string_view dirPath,
                                             std::string_view requestUri,
                                             const RequestRouter *router) {

  if (router) {
    const LocationBlock *location = router->findLocation(std::string(requestUri));
    std::string configuredIndex = router->getIndexFile(location);

    std::string indexPath = std::string(dirPath) + "/" + configuredIndex;

    if (FileUtils::exists(indexPath) &&
        !std::filesystem::is_directory(indexPath)) {
      return indexPath;
    }
  }

  const char *indexFiles[] = {"index.html", "index.htm", nullptr};
  for (const char **indexFile = indexFiles; *indexFile; ++indexFile) {
    std::string indexPath = std::string(dirPath) + "/" + *indexFile;

    if (FileUtils::exists(indexPath) &&
        !std::filesystem::is_directory(indexPath)) {
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
      "<p>Please create an index.html file in your root "
      "directory.</p></body></html>";

  return HttpResponse::ok(welcomePage, "text/html");
}
