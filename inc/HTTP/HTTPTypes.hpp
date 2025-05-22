#pragma once

#include <string>
#include <map>

namespace HTTP {
    // HTTP Methods
    enum class Method {
        GET,
        POST,
        DELETE
    };
    
    // HTTP Status Codes
    enum class StatusCode {
        OK = 200,
        BAD_REQUEST = 400,
        NOT_FOUND = 404,
        METHOD_NOT_ALLOWED = 405,
        INTERNAL_SERVER_ERROR = 500,
        NOT_IMPLEMENTED = 501
    };
    
    // HTTP Version
    enum class Version {
        HTTP_1_0,
        HTTP_1_1
    };
    
    // Request Line structure
    struct RequestLine {
        Method method;
        std::string uri;
        std::string version;
    };
    
    // Helper functions
    Method stringToMethod(const std::string& method);
    std::string methodToString(Method method);
    std::string statusToString(StatusCode status);
    std::string versionToString(Version version);
    
    // Validation functions
    bool isValidMethod(const std::string& method);
    bool isValidUri(const std::string& uri);
    bool isValidVersion(const std::string& version);
}