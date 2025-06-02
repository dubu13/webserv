#pragma once
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <string_view>

namespace HTTP {

    enum class Method {
        GET,
        POST,
        DELETE,
        UNKNOWN
    };

    enum class StatusCode {
        OK = 200,
        CREATED = 201,
        NO_CONTENT = 204,
        MOVED_PERMANENTLY = 301,
        FOUND = 302,
        BAD_REQUEST = 400,
        FORBIDDEN = 403,
        NOT_FOUND = 404,
        METHOD_NOT_ALLOWED = 405,
        CONFLICT = 409,
        PAYLOAD_TOO_LARGE = 413,
        INTERNAL_SERVER_ERROR = 500,
        NOT_IMPLEMENTED = 501
    };

    inline Method stringToMethod(const std::string& methodStr) {
        if (methodStr == "GET")    return Method::GET;
        if (methodStr == "POST")   return Method::POST;
        if (methodStr == "DELETE") return Method::DELETE;
        return Method::UNKNOWN;
    }

    inline std::string methodToString(Method method) {
        switch (method) {
            case Method::GET:    return "GET";
            case Method::POST:   return "POST";
            case Method::DELETE: return "DELETE";
            default:            return "UNKNOWN";
        }
    }

    inline std::string statusToString(StatusCode status) {
        static const std::unordered_map<StatusCode, std::string_view> statusToStringMap = {
            {StatusCode::OK, "OK"},
            {StatusCode::CREATED, "Created"},
            {StatusCode::NO_CONTENT, "No Content"},
            {StatusCode::MOVED_PERMANENTLY, "Moved Permanently"},
            {StatusCode::FOUND, "Found"},
            {StatusCode::BAD_REQUEST, "Bad Request"},
            {StatusCode::FORBIDDEN, "Forbidden"},
            {StatusCode::NOT_FOUND, "Not Found"},
            {StatusCode::METHOD_NOT_ALLOWED, "Method Not Allowed"},
            {StatusCode::CONFLICT, "Conflict"},
            {StatusCode::PAYLOAD_TOO_LARGE, "Payload Too Large"},
            {StatusCode::INTERNAL_SERVER_ERROR, "Internal Server Error"},
            {StatusCode::NOT_IMPLEMENTED, "Not Implemented"}
        };

        auto it = statusToStringMap.find(status);
        if (it != statusToStringMap.end()) {
            return std::string(it->second);
        }
        return "Unknown";
    }

    inline std::string getMimeType(const std::string& path) {
        static const std::unordered_map<std::string_view, std::string_view> extensionToMimeMap = {
            {"html", "text/html"},
            {"htm", "text/html"},
            {"css", "text/css"},
            {"js", "text/javascript"},
            {"json", "application/json"},
            {"txt", "text/plain"},
            {"png", "image/png"},
            {"jpg", "image/jpeg"},
            {"jpeg", "image/jpeg"},
            {"gif", "image/gif"},
            {"pdf", "application/pdf"},
            {"ico", "image/x-icon"},
            {"svg", "image/svg+xml"},
            {"xml", "application/xml"},
            {"zip", "application/zip"},
            {"mp3", "audio/mpeg"},
            {"mp4", "video/mp4"}
        };

        size_t dotPos = path.find_last_of('.');
        if (dotPos == std::string::npos || dotPos == path.length() - 1) {
            return "application/octet-stream";
        }

        std::string ext = path.substr(dotPos + 1);

        for (char& c : ext) {
            if (c >= 'A' && c <= 'Z') c += 32;
        }

        auto it = extensionToMimeMap.find(ext);
        return (it != extensionToMimeMap.end()) ? std::string(it->second) : "application/octet-stream";
    }

}
