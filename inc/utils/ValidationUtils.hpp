#pragma once
#include "HTTP/core/HTTPTypes.hpp"
#include "Constants.hpp"
#include <string>

class ValidationUtils {
public:
    struct FileAccessResult {
        std::string filePath;
        HTTP::StatusCode status;
        bool isValid;
    };

    static FileAccessResult validateFileAccess(std::string_view rootDir,
                                             std::string_view uri);

    static bool validateLimit(size_t value, size_t limit, const char* errorMsg);
    static bool validateContentLength(const std::string& length, size_t& result);
    static bool validateBodySize(size_t size);
    static bool validateChunkCount(size_t count);
    static bool validateChunkSize(size_t size);
    static bool validateHeaderLength(size_t length);
    static bool validateUriLength(size_t length);
    static bool validateHeaderSize(const std::string& data, size_t maxSize);
    static bool validateChunkTerminator(std::string_view data, size_t pos);

    static bool isPathSafe(std::string_view path);
};
