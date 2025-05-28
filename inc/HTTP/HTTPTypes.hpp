#pragma once
#include <string>
#include <map>
namespace HTTP {
    enum class Method {
        GET,
        POST,
        DELETE
    };
    enum class StatusCode {
        OK = 200,
        BAD_REQUEST = 400,
        NOT_FOUND = 404,
        METHOD_NOT_ALLOWED = 405,
        INTERNAL_SERVER_ERROR = 500,
        NOT_IMPLEMENTED = 501
    };
    enum class Version {
        HTTP_1_0,
        HTTP_1_1
    };
    struct RequestLine {
        Method method;
        std::string uri;
        std::string version;
    };
    Method stringToMethod(const std::string& method);
    std::string methodToString(Method method);
    std::string statusToString(StatusCode status);
    std::string versionToString(Version version);
}