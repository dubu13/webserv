#pragma once
#include "config/ServerBlock.hpp"
#include <string>

class ErrorResponseBuilder {
public:
  static void setCurrentConfig(const ServerBlock *config);
  static std::string buildResponse(int statusCode);
  static std::string buildDefaultError(int statusCode);

private:
  static std::string loadCustomErrorPage(int statusCode);
  static const ServerBlock *_currentConfig;
};
