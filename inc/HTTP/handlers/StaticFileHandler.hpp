#pragma once
#include "utils/Utils.hpp"
#include <memory>
#include <string>
#include <string_view>

// Forward declaration
class RequestRouter;

class StaticFileHandler {
public:
  static std::string handleRequest(std::string_view root, std::string_view uri,
                                   const RequestRouter *router = nullptr);
private:
  static std::string serveFile(std::string_view filePath);
  static std::string serveDirectory(std::string_view dirPath,
                                    std::string_view requestUri,
                                    const RequestRouter *router = nullptr);
  static std::string findIndexFile(std::string_view dirPath,
                                   std::string_view requestUri,
                                   const RequestRouter *router = nullptr);
  static std::string generateWelcomePage();
};
