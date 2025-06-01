#pragma once

#include "HTTP/HTTPTypes.ipp"
#include <string>
#include <map>

namespace HTTP {

class ResponseBuilder {
private:
    HTTP::StatusCode _status;
    std::map<std::string, std::string> _headers;
    std::string _body;
    bool _keepAlive;
    bool _chunked;

    std::string buildStatusLine() const;
    std::string buildHeaders() const;
    void setStandardHeaders();

public:
    ResponseBuilder(HTTP::StatusCode status = HTTP::StatusCode::OK);
    
    // Builder pattern methods
    ResponseBuilder& setStatus(HTTP::StatusCode status);
    ResponseBuilder& setHeader(const std::string& name, const std::string& value);
    ResponseBuilder& setContentType(const std::string& type);
    ResponseBuilder& setBody(const std::string& body);
    ResponseBuilder& setKeepAlive(bool keepAlive = true);
    ResponseBuilder& setChunked(bool chunked = true);
    ResponseBuilder& setCacheControl(const std::string& control);
    
    // Build final response
    std::string build();
    
    // Static convenience methods for common responses
    static std::string createSimpleResponse(HTTP::StatusCode status, const std::string& message);
    static std::string createErrorResponse(HTTP::StatusCode status, const std::string& errorMessage = "");
    static std::string createFileResponse(HTTP::StatusCode status, const std::string& content, const std::string& contentType);
    static std::string createRedirectResponse(HTTP::StatusCode status, const std::string& location);
    static std::string createChunkedFileResponse(HTTP::StatusCode status, const std::string& content, const std::string& contentType);
};

} // namespace HTTP
