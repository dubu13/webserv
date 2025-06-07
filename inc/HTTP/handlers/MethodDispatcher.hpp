#pragma once

#include "HTTP/core/HTTPParser.hpp"
#include "HTTP/core/HTTPTypes.hpp"
#include "HTTP/core/HttpResponse.hpp"
#include "HTTP/routing/RequestRouter.hpp"
#include "utils/Logger.hpp"
#include <string>
#include <string_view>

using HTTP::Request;

class MethodHandler {
public:
  static std::string handleRequest(const Request &request,
                                   std::string_view root = "./www",
                                   const RequestRouter *router = nullptr);
private:
  static std::pair<std::string, std::string>
  resolvePaths(const Request &request, std::string_view root,
               const RequestRouter *router = nullptr);
  static std::string handleGet(const Request &request, std::string_view root,
                               const RequestRouter *router = nullptr);
  static std::string handlePost(const Request &request, std::string_view root,
                                const RequestRouter *router = nullptr);
  static std::string handleDelete(const Request &request, std::string_view root,
                                  const RequestRouter *router = nullptr);
  static std::string handleFileUpload(const Request &request, const std::string &effectiveRoot,
                                     std::string_view contentType, const RequestRouter *router);
};
