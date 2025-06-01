#pragma once
#include "HTTP/HTTPTypes.hpp"
#include "HTTP/HTTPParser.hpp"
#include "config/ServerBlock.ipp"
#include "HTTP/HTTPResponseBuilder.hpp"
#include "utils/FileUtils.hpp"
#include "utils/Logger.hpp"
#include <map>
#include <string>
#include <functional>

// Forward declaration
struct LocationBlock;

namespace HTTP {

constexpr size_t CHUNKED_THRESHOLD = 1024 * 1024;

// Function type for method handlers
using MethodHandler = std::function<std::string(const HTTP::Request&, const std::string&, const std::string&, const LocationBlock*)>;

namespace RequestValidator {
    bool isMethodAllowed(const HTTP::Request& request, const LocationBlock* location);
    bool hasRequiredAuth(const HTTP::Request& request);
    std::string getHeaderValue(const HTTP::Request& request, const std::string& headerName);
}

namespace PathResolver {
    std::string resolveIndexFile(const std::string& dirPath, const LocationBlock* location);
    std::string cleanRequestUri(const std::string& uri);
    std::string handleDirectoryRequest(const std::string& filePath, 
                                     const std::string& cleanUri, 
                                     const LocationBlock* location,
                                     std::string& updatedFilePath,
                                     std::string& updatedCleanUri);
}

// HTTP method handlers
std::string handleGetRequest(const HTTP::Request& request, 
                           const std::string& effectiveRoot,
                           const std::string& effectiveUri, 
                           const LocationBlock* location);

std::string handlePostRequest(const HTTP::Request& request, 
                            const std::string& effectiveRoot, 
                            const std::string& effectiveUri, 
                            const LocationBlock* location);

std::string handleDeleteRequest(const HTTP::Request& request, 
                              const std::string& effectiveRoot, 
                              const std::string& effectiveUri, 
                              const LocationBlock* location);

// Request processing utilities
std::pair<std::string, std::string> resolveEffectivePath(const std::string& uri, 
                                                        const std::string& rootDirectory, 
                                                        const LocationBlock* location);

// Error handling utilities
std::string handleErrorWithCustomPage(HTTP::StatusCode status, 
                                    const std::map<HTTP::StatusCode, std::string>& customErrorPages,
                                    const std::string& rootDirectory);

// Method dispatcher
const std::map<HTTP::Method, MethodHandler>& getMethodHandlers();

std::string dispatchMethodHandler(const HTTP::Request& request, 
                                const std::string& effectiveRoot,
                                const std::string& effectiveUri, 
                                const LocationBlock* location);

} // namespace HTTP
