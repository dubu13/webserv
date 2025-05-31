#pragma once

#include "HTTP/HTTPTypes.hpp"
#include <string>
#include <map>

namespace HttpResponseBuilder {
    // Focused on HTTP response building only
    
    // Response building utilities
    std::string buildResponse(HTTP::StatusCode status, const std::map<std::string, std::string>& headers, 
                             const std::string& body, bool keepAlive = false);
    
    // Convenience response builders
    std::string createSimpleResponse(HTTP::StatusCode status, const std::string& message);
    std::string createErrorResponse(HTTP::StatusCode status, const std::string& errorMessage = "");
    std::string createFileResponse(HTTP::StatusCode status, const std::string& content, const std::string& contentType);
}
