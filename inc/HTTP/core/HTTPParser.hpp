#pragma once

#include <string>
#include <string_view>
#include <map>
#include "HTTPTypes.hpp"
#include "utils/Utils.hpp"
#include "utils/Logger.hpp"

namespace HTTP {

    struct RequestLine {
        Method method;
        std::string uri;
        std::string version;
    };

    struct Request {
        RequestLine requestLine;
        std::map<std::string, std::string> headers;
        std::string body;
        bool keepAlive = false;
        bool isChunked = false;

        size_t contentLength = 0;
        bool chunkedTransfer = false;
        std::string contentType;
    };

    bool parseRequest(const std::string& data, Request& request);
    bool parseRequestLine(std::string_view line, RequestLine& requestLine);

    bool parseHeaders(std::string_view headerSection, std::map<std::string, std::string>& headers);
    bool parseHeader(std::string_view line, std::map<std::string, std::string>& headers);

    bool parseBody(std::string_view data, size_t bodyStart, std::string& body);
    bool parseChunkedBody(std::string_view data, size_t bodyStart, std::string& body);
    bool parseChunk(std::string_view data, size_t& pos, std::string& body);

    bool setKeepAlive(const std::map<std::string, std::string>& headers, const std::string& version);

    bool validateHttpRequest(const Request& request);

    std::string_view trimWhitespace(std::string_view str);
    std::pair<std::string_view, std::string_view> splitFirst(std::string_view str, char delimiter);
    bool parseHexNumber(std::string_view hex, size_t& result);

    constexpr size_t MAX_URI_LENGTH = 2048;
    constexpr size_t MAX_CONTENT_LENGTH = 10 * 1024 * 1024;
    constexpr size_t MAX_HEADER_SIZE = 8 * 1024;
    constexpr size_t MAX_HEADERS = 100;
    constexpr size_t MAX_CHUNK_SIZE = 1024 * 1024;
    constexpr size_t MAX_TOTAL_SIZE = 10 * 1024 * 1024;
    constexpr size_t MAX_CHUNKS = 1024;

}
