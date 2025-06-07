#pragma once
#include "Constants.hpp"
#include "HTTP/core/HTTPTypes.hpp"
#include <string>

class ValidationUtils {
public:
  static bool validateLimit(size_t value, size_t limit, const char *errorMsg);
  static bool validateContentLength(const std::string &length, size_t &result, size_t maxSize = Constants::MAX_TOTAL_SIZE);
  static bool validateHeaderSize(const std::string &data, size_t maxSize);
  static bool validateChunkTerminator(std::string_view data, size_t pos);
  static bool isPathSafe(std::string_view path);
};
