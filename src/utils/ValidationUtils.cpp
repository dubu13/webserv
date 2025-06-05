#include "utils/ValidationUtils.hpp"
#include "utils/Utils.hpp"
#include "utils/Logger.hpp"
#include "HTTP/core/HTTPTypes.hpp"
#include <string_view>

using namespace HTTP;

ValidationUtils::FileAccessResult
ValidationUtils::validateFileAccess(std::string_view rootDir, std::string_view uri) {
    FileAccessResult result;
    result.filePath = HttpUtils::buildPath(rootDir, uri);

    if (!isPathSafe(uri)) {
        result.status = StatusCode::FORBIDDEN;
        result.isValid = false;
        return result;
    }

    result.status = StatusCode::OK;
    result.isValid = true;
    return result;
}

bool ValidationUtils::validateLimit(size_t value, size_t limit, const char* errorMsg) {
    if (value > limit) {
        Logger::error(errorMsg);
        return false;
    }
    return true;
}

bool ValidationUtils::validateContentLength(const std::string& length, size_t& result) {
    try {
        result = std::stoull(length);
        if (!validateBodySize(result)) {
            return false;
        }
        return true;
    } catch (...) {
        Logger::error("HTTP/1.1 Error: Invalid Content-Length format");
        return false;
    }
}

bool ValidationUtils::validateBodySize(size_t size) {
    return validateLimit(size, Constants::MAX_TOTAL_SIZE,
                     "HTTP/1.1 Error: Body size too large (max 10MB)");
}

bool ValidationUtils::validateChunkCount(size_t count) {
    return validateLimit(count, Constants::MAX_CHUNK_COUNT,
                     "HTTP/1.1 Error: Too many chunks");
}

bool ValidationUtils::validateChunkSize(size_t size) {
    return validateLimit(size, Constants::MAX_CHUNK_SIZE,
                     "HTTP/1.1 Error: Chunk size too large");
}

bool ValidationUtils::validateHeaderLength(size_t length) {
    return validateLimit(length, Constants::MAX_HEADER_SIZE,
                     "HTTP/1.1 Error: Headers too large");
}

bool ValidationUtils::validateUriLength(size_t length) {
    return validateLimit(length, Constants::MAX_URI_LENGTH,
                     "HTTP/1.1 Error: URI too long");
}

bool ValidationUtils::validateHeaderSize(const std::string& data, size_t maxSize) {
    size_t headerEnd = data.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return data.length() <= maxSize;
    }
    return headerEnd <= maxSize;
}

bool ValidationUtils::validateChunkTerminator(std::string_view data, size_t pos) {
    if (pos + 1 >= data.length() || data[pos] != '\r' || data[pos + 1] != '\n') {
        Logger::error("HTTP/1.1 Error: Invalid chunk terminator");
        return false;
    }
    return true;
}

bool ValidationUtils::isPathSafe(std::string_view path) {
    std::string pathStr(path);

    if (pathStr.find("../") != std::string::npos ||
        pathStr.find("..\\") != std::string::npos ||
        pathStr.find("/..") != std::string::npos ||
        pathStr.find("\\..") != std::string::npos) {
        Logger::logf<LogLevel::WARN>("Path contains directory traversal: %", pathStr);
        return false;
    }

    if (pathStr.find('\0') != std::string::npos) {
        Logger::logf<LogLevel::WARN>("Path contains null byte: %", pathStr);
        return false;
    }

    return true;
}
