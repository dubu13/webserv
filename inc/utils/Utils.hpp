#pragma once
#include "HTTP/core/HTTPTypes.hpp"
#include "Constants.hpp"
#include <string>
#include <string_view>
#include <optional>
#include <filesystem>

using HTTP::StatusCode;

class HttpUtils {
public:

    static std::string getEffectiveRoot(std::string_view root);

    static std::string_view trimWhitespace(std::string_view str);
    static std::string urlDecode(std::string_view encoded);

    static bool parseHexNumber(std::string_view hex, size_t& result);

    static bool parseChunkSize(std::string_view data, size_t& pos, size_t& chunkSize);
    static bool findChunkEnd(std::string_view data, size_t& pos);

    static bool isCompleteRequest(const std::string& data);
    static bool isSecureRequest(const std::string& data);

    static std::string buildPath(std::string_view root, std::string_view path);
    static std::string sanitizePath(std::string_view path);
    static std::filesystem::path canonicalizePath(std::string_view path);
    static std::string extractQueryParams(std::string_view uri);
    static std::string cleanUri(std::string_view uri);
};

class FileUtils {
public:

    static std::string readFile(std::string_view rootDir, std::string_view uri, StatusCode &status);
    static bool writeFile(std::string_view rootDir, std::string_view uri, std::string_view content, StatusCode &status);
    static bool deleteFile(std::string_view rootDir, std::string_view uri, StatusCode &status);

    static std::optional<std::string> readFileContent(std::string_view filePath);
    static bool writeFileContent(std::string_view filePath, std::string_view content);

    static bool isDirectory(std::string_view path);
    static std::string generateDirectoryListing(std::string_view dirPath, std::string_view uri);
    static bool createDirectories(std::string_view path);

    static bool exists(std::string_view path);

    static std::string getMimeType(std::string_view filePath);

    static void clearCache();
    static void setCacheMaxSize(size_t maxSize);
};
