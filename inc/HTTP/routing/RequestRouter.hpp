#pragma once

#include <string>
#include <string_view>
#include "config/ServerBlock.hpp"
#include "config/LocationBlock.hpp"
#include "HTTP/core/HTTPTypes.hpp"
#include "HTTP/core/HTTPParser.hpp"

using HTTP::Request;
using HTTP::methodToString;

class RequestRouter {
private:
    const ServerBlock* _config;

public:
    explicit RequestRouter(const ServerBlock* config);

    const LocationBlock* findLocation(const std::string& uri) const;
    std::string resolveRoot(const std::string_view uri, const LocationBlock* location) const;
    bool isMethodAllowed(const Request& request, const LocationBlock* location) const;
};
