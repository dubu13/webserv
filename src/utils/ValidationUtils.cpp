#include "utils/ValidationUtils.hpp"
#include "HTTP/core/HTTPTypes.hpp"
#include "HTTP/routing/RequestRouter.hpp"
#include "config/LocationBlock.hpp"
#include "config/ServerBlock.hpp"
#include "utils/Logger.hpp"
#include "utils/Utils.hpp"
#include <string_view>

using namespace HTTP;

bool ValidationUtils::validateLimit(size_t value, size_t limit,
                                    const char *errorMsg) {
  if (value > limit) {
    Logger::error(errorMsg);
    return false;
  }
  return true;
}

bool ValidationUtils::validateContentLength(const std::string &length,
                                            size_t &result, size_t maxSize) {
  try {
    result = std::stoull(length);
    if (!validateLimit(result, maxSize, "HTTP/1.1 Error: Body size too large")) {
      return false;
    }
    return true;
  } catch (...) {
    Logger::error("HTTP/1.1 Error: Invalid Content-Length format");
    return false;
  }
}

bool ValidationUtils::validateContentLengthWithRouter(const std::string &length, const std::string &uri, size_t &result, const RequestRouter *router) {
  try {
    result = std::stoull(length);
    size_t maxSize = getMaxBodySize(uri, router);
    if (!validateLimit(result, maxSize, "HTTP/1.1 Error: Body size too large")) {
      return false;
    }
    return true;
  } catch (...) {
    Logger::error("HTTP/1.1 Error: Invalid Content-Length format");
    return false;
  }
}

size_t ValidationUtils::getMaxBodySize(const std::string &uri, const RequestRouter *router) {
  if (!router) {
    return Constants::MAX_TOTAL_SIZE;
  }
  
  const LocationBlock *location = router->findLocation(uri);
  if (location && location->clientMaxBodySize > 0) {
    return location->clientMaxBodySize;
  }
  
  const ServerBlock *serverConfig = router->getConfig();
  if (serverConfig && serverConfig->clientMaxBodySize > 0) {
    return serverConfig->clientMaxBodySize;
  }
  
  return Constants::MAX_TOTAL_SIZE;
}

bool ValidationUtils::validateHeaderSize(const std::string &data,
                                         size_t maxSize) {
  size_t headerEnd = data.find("\r\n\r\n");
  if (headerEnd == std::string::npos) {
    return data.length() <= maxSize;
  }
  return headerEnd <= maxSize;
}

bool ValidationUtils::validateChunkTerminator(std::string_view data,
                                              size_t pos) {
  if (pos + 1 >= data.length() || data[pos] != '\r' || data[pos + 1] != '\n') {
    Logger::error("HTTP/1.1 Error: Invalid chunk terminator");
    return false;
  }
  return true;
}

bool ValidationUtils::isPathSafe(std::string_view path) {
  std::string pathStr(path);

  if (pathStr.find("../") != std::string::npos ||
      pathStr.find("..\\") != std::string::npos ||
      pathStr.find("/..") != std::string::npos ||
      pathStr.find("\\..") != std::string::npos) {
    Logger::logf<LogLevel::WARN>("Path contains directory traversal: %s",
                                 pathStr);
    return false;
  }

  if (pathStr.find('\0') != std::string::npos) {
    Logger::logf<LogLevel::WARN>("Path contains null byte: %s", pathStr);
    return false;
  }

  return true;
}
