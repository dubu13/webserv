#include "utils/Utils.hpp"
#include "utils/ValidationUtils.hpp"
#include "utils/Logger.hpp"
#include "HTTP/core/HTTPParser.hpp"
#include <algorithm>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <map>
#include <filesystem>

std::string HttpUtils::getEffectiveRoot(std::string_view root) {
    return root.empty() ? "./www" : std::string(root);
}

std::string_view HttpUtils::trimWhitespace(std::string_view str) {
    const char* whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return {};
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::string HttpUtils::urlDecode(std::string_view encoded) {
    std::string result;
    result.reserve(encoded.size());

    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {

            char hex[3] = {encoded[i + 1], encoded[i + 2], '\0'};
            char* end;
            unsigned long value = std::strtoul(hex, &end, 16);

            if (end == hex + 2) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += encoded[i];
            }
        } else if (encoded[i] == '+') {
            result += ' ';
        } else {
            result += encoded[i];
        }
    }

    return result;
}

bool HttpUtils::parseHexNumber(std::string_view hex, size_t& result) {
    result = 0;
    for (char c : hex) {
        result *= 16;
        if (c >= '0' && c <= '9') {
            result += c - '0';
        } else if (c >= 'a' && c <= 'f') {
            result += c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            result += c - 'A' + 10;
        } else {
            return false;
        }
    }
    return true;
}

bool HttpUtils::parseChunkSize(std::string_view data, size_t& pos, size_t& chunkSize) {
    size_t lineEnd = data.find("\r\n", pos);
    if (lineEnd == std::string::npos) {
        Logger::error("HTTP/1.1 Error: Missing chunk size");
        return false;
    }

    std::string_view sizeLine = data.substr(pos, lineEnd - pos);
    if (!parseHexNumber(sizeLine, chunkSize)) {
        Logger::error("HTTP/1.1 Error: Invalid chunk size format");
        return false;
    }

    if (!ValidationUtils::validateChunkSize(chunkSize)) {
        return false;
    }

    pos = lineEnd + 2;
    return true;
}

bool HttpUtils::findChunkEnd(std::string_view data, size_t& pos) {
    size_t end = data.find("\r\n", pos);
    if (end == std::string::npos) {
        Logger::error("HTTP/1.1 Error: Missing chunk terminator");
        return false;
    }
    pos = end + 2;
    return true;
}

bool HttpUtils::isCompleteRequest(const std::string& data) {

    size_t headerEnd = data.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return false;
    }

    size_t contentLengthPos = data.find("Content-Length: ");
    if (contentLengthPos != std::string::npos && contentLengthPos < headerEnd) {
        size_t valueStart = contentLengthPos + 16;
        size_t valueEnd = data.find("\r\n", valueStart);
        if (valueEnd != std::string::npos) {
            try {
                size_t contentLength = std::stoull(data.substr(valueStart, valueEnd - valueStart));
                return data.length() >= headerEnd + 4 + contentLength;
            } catch (...) {
                return false;
            }
        }
    }

    size_t transferEncodingPos = data.find("Transfer-Encoding: chunked");
    if (transferEncodingPos != std::string::npos && transferEncodingPos < headerEnd) {

        return data.find("0\r\n\r\n", headerEnd + 4) != std::string::npos;
    }

    return true;
}

bool HttpUtils::isSecureRequest(const std::string& data) {

    size_t firstLine = data.find("\r\n");
    if (firstLine != std::string::npos) {
        std::string_view requestLine(data.data(), firstLine);

        if (requestLine.find("..") != std::string::npos) return false;

        if (requestLine.find('\0') != std::string::npos) return false;
    }
    return true;
}

std::string HttpUtils::sanitizePath(std::string_view path) {
    if (path.empty()) return "/";

    try {
        std::filesystem::path fsPath(path);
        fsPath = fsPath.lexically_normal();

        std::string result = fsPath.string();
        if (result.empty() || result[0] != '/') {
            result = "/" + result;
        }

        return result;
    } catch (const std::exception&) {
        return "/";
    }
}

std::string HttpUtils::buildPath(std::string_view root, std::string_view path) {
    std::string rootStr(root);
    std::string pathStr(path);

    if (!rootStr.empty() && rootStr.back() == '/') {
        rootStr.pop_back();
    }

    if (pathStr.empty() || pathStr[0] != '/') {
        pathStr = "/" + pathStr;
    }

    std::string fullPath = rootStr + pathStr;

    return fullPath;
}

std::filesystem::path HttpUtils::canonicalizePath(std::string_view path) {
    try {
        return std::filesystem::canonical(std::filesystem::path(path));
    } catch (const std::filesystem::filesystem_error&) {
        return std::filesystem::path{};
    }
}

std::string HttpUtils::extractQueryParams(std::string_view uri) {
    const size_t queryPos = uri.find('?');
    return (queryPos != std::string_view::npos) ? std::string(uri.substr(queryPos + 1)) : "";
}

std::string HttpUtils::cleanUri(std::string_view uri) {
    const size_t queryPos = uri.find('?');
    return (queryPos != std::string_view::npos) ? std::string(uri.substr(0, queryPos)) : std::string(uri);
}
