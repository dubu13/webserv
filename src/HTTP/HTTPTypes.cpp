#include "HTTPTypes.hpp"
#include <stdexcept>

namespace HTTP {
    Method stringToMethod(const std::string& method) {
        if (method == "GET") return Method::GET;
        if (method == "POST") return Method::POST;
        if (method == "DELETE") return Method::DELETE;
        // Default return GET or throw an exception
        throw std::runtime_error("Invalid HTTP method: " + method);
    }

    std::string methodToString(Method method) {
        switch (method) {
            case Method::GET: return "GET";
            case Method::POST: return "POST";
            case Method::DELETE: return "DELETE";
            default: return "UNKNOWN";
        }
    }

    std::string statusToString(StatusCode status) {
        switch (status) {
            case StatusCode::OK: return "OK";
            case StatusCode::BAD_REQUEST: return "Bad Request";
            case StatusCode::NOT_FOUND: return "Not Found";
            case StatusCode::METHOD_NOT_ALLOWED: return "Method Not Allowed";
            case StatusCode::INTERNAL_SERVER_ERROR: return "Internal Server Error";
            default: return "Unknown";
        }
    }

    std::string versionToString(Version version) {
        switch (version) {
            case Version::HTTP_1_0: return "HTTP/1.0";
            case Version::HTTP_1_1: return "HTTP/1.1";
            default: return "HTTP/1.1";
        }
    }

    bool isValidMethod(const std::string& method) {
        return method == "GET" || method == "POST" || method == "DELETE";
    }

    bool isValidUri(const std::string& uri) {
        return !uri.empty() && uri[0] == '/';
    }

    bool isValidVersion(const std::string& version) {
        return version == "HTTP/1.0" || version == "HTTP/1.1";
    }
}