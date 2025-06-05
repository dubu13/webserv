#pragma once
#include <string>
#include "config/ServerBlock.hpp"

class ErrorResponseBuilder {
public:
    static void setCurrentConfig(const ServerBlock* config);
    static std::string buildResponse(int statusCode);
    static std::string buildDefaultError(int statusCode);

private:
    static std::string loadCustomErrorPage(int statusCode);
    static const ServerBlock* _currentConfig;
};
