#pragma once

#include <map>
#include <string>
#include <vector>

class ConfigUtils {
public:
  static std::vector<std::string> splitWhitespace(const std::string &str);

  static std::vector<std::string>
  extractServerBlocks(const std::string &content);

  static std::pair<std::string, std::string>
  parseDirective(const std::string &line);
  static std::vector<std::string> parseMultiValue(const std::string &value);

  static bool isValidIPv4(const std::string &ip);
  static bool isValidMethod(const std::string &method);
  static bool isValidServerName(const std::string &name);
  static bool isValidPath(const std::string &path);

  static size_t parseSize(const std::string &value);
  static bool parseBooleanValue(const std::string &value);
  static std::pair<std::string, int>
  parseListenDirective(const std::string &value);
  static std::map<int, std::string> parseErrorPages(const std::string &value);
};
