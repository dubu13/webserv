#pragma once

#include <string>
#include <string_view>
#include "HTTP/core/HTTPTypes.hpp"
#include "HTTP/core/HTTPParser.hpp"
#include "HTTP/core/HttpResponse.hpp"
#include "HTTP/routing/RequestRouter.hpp"
#include "utils/Logger.hpp"

using HTTP::Request;

class MethodHandler {
public:

    static std::string handleRequest(const Request& request, std::string_view root = "./www", const RequestRouter* router = nullptr);

private:

    static std::string handleGet(const Request& request, std::string_view root);
    static std::string handlePost(const Request& request, std::string_view root);
    static std::string handleDelete(const Request& request, std::string_view root);
};
