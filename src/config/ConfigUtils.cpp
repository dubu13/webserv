#include "config/ConfigUtils.hpp"
#include "utils/Utils.hpp"
#include "utils/ValidationUtils.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string_view>

std::vector<std::string> ConfigUtils::splitWhitespace(const std::string &str) {
  std::vector<std::string> tokens;
  std::istringstream iss(str);
  std::string token;
  while (iss >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

std::vector<std::string>
ConfigUtils::extractServerBlocks(const std::string &content) {
  std::vector<std::string> blocks;
  std::istringstream iss(content);
  std::string line;

  while (std::getline(iss, line)) {
    line = std::string(HttpUtils::trimWhitespace(line));
    if (line == "server {") {
      std::string blockContent;
      int braceCount = 1;

      while (std::getline(iss, line) && braceCount > 0) {
        for (char c : line) {
          if (c == '{')
            braceCount++;
          else if (c == '}')
            braceCount--;
        }
        if (braceCount > 0) {
          blockContent += line + "\n";
        }
      }
      blocks.push_back(blockContent);
    }
  }
  return blocks;
}

std::pair<std::string, std::string>
ConfigUtils::parseDirective(const std::string &line) {
  std::string_view trimmed_view = HttpUtils::trimWhitespace(line);

  if (trimmed_view.empty() || trimmed_view[0] == '#') {
    return {"", ""};
  }

  size_t space_pos = trimmed_view.find(' ');
  if (space_pos == std::string_view::npos) {
    return {std::string(trimmed_view), ""};
  }

  std::string_view directive_view = trimmed_view.substr(0, space_pos);
  std::string_view value_view =
      HttpUtils::trimWhitespace(trimmed_view.substr(space_pos + 1));

  if (!value_view.empty() && value_view.back() == ';') {
    value_view.remove_suffix(1);
    value_view = HttpUtils::trimWhitespace(value_view);
  }

  return {std::string(directive_view), std::string(value_view)};
}

std::vector<std::string>
ConfigUtils::parseMultiValue(const std::string &value) {
  return ConfigUtils::splitWhitespace(value);
}

size_t ConfigUtils::parseSize(const std::string &value) {
  if (value.empty()) {
    throw std::invalid_argument("Empty size value");
  }

  std::string numStr = value;
  size_t multiplier = 1;

  char last = std::tolower(value.back());
  if (last == 'k') {
    multiplier = 1024;
    numStr.pop_back();
  } else if (last == 'm') {
    multiplier = 1024 * 1024;
    numStr.pop_back();
  } else if (last == 'g') {
    multiplier = 1024 * 1024 * 1024;
    numStr.pop_back();
  }

  try {
    size_t num = std::stoull(numStr);
    return num * multiplier;
  } catch (const std::exception &) {
    throw std::invalid_argument("Invalid size format");
  }
}

bool ConfigUtils::parseBooleanValue(const std::string &value) {
  std::string lower = value;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  return lower == "on" || lower == "true" || lower == "yes" || lower == "1";
}

std::pair<std::string, int>
ConfigUtils::parseListenDirective(const std::string &value) {
  std::istringstream iss(value);
  std::string token;

  if (!(iss >> token)) {
    throw std::invalid_argument("Empty listen directive");
  }

  size_t colonPos = token.find(':');
  if (colonPos == std::string::npos) {
    try {
      int port = std::stoi(token);
      if (port < 1 || port > 65535) {
        throw std::invalid_argument("Invalid port range");
      }
      return {"0.0.0.0", port};
    } catch (const std::exception &) {
      throw std::invalid_argument("Invalid port format");
    }
  }

  std::string host = token.substr(0, colonPos);
  std::string portStr = token.substr(colonPos + 1);

  if (host.empty())
    host = "0.0.0.0";
  if (!ConfigUtils::isValidIPv4(host)) {
    throw std::invalid_argument("Invalid IP address");
  }

  try {
    int port = std::stoi(portStr);
    if (port < 1 || port > 65535) {
      throw std::invalid_argument("Invalid port range");
    }
    return {host, port};
  } catch (const std::exception &) {
    throw std::invalid_argument("Invalid port format");
  }
}

std::map<int, std::string>
ConfigUtils::parseErrorPages(const std::string &value) {
  std::map<int, std::string> errorPages;
  std::istringstream iss(value);
  std::vector<std::string> codes;
  std::string token;

  while (iss >> token) {
    codes.push_back(token);
  }

  if (codes.size() < 2) {
    throw std::invalid_argument("Invalid error_page format");
  }

  std::string path = codes.back();
  for (size_t i = 0; i < codes.size() - 1; ++i) {
    try {
      int code = std::stoi(codes[i]);
      if (code < 100 || code > 599) {
        throw std::invalid_argument("Invalid HTTP status code");
      }
      errorPages[code] = path;
    } catch (const std::exception &) {
      throw std::invalid_argument("Invalid HTTP status code format");
    }
  }

  return errorPages;
}

bool ConfigUtils::isValidIPv4(const std::string &ip) {
  struct sockaddr_in sa;
  return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) != 0;
}

bool ConfigUtils::isValidMethod(const std::string &method) {

  static const std::vector<std::string> configValidMethods = {"GET", "POST",
                                                              "DELETE"};
  return std::find(configValidMethods.begin(), configValidMethods.end(),
                   method) != configValidMethods.end();
}

bool ConfigUtils::isValidServerName(const std::string &name) {
  if (name.empty())
    return false;
  if (name == "*")
    return true;

  if (name.size() > 2 && name.substr(0, 2) == "*.") {
    return ConfigUtils::isValidServerName(name.substr(2));
  }

  for (char c : name) {
    if (!std::isalnum(c) && c != '.' && c != '-') {
      return false;
    }
  }
  return true;
}

bool ConfigUtils::isValidPath(const std::string &path) {
  if (path.empty()) {
    return false;
  }

  if (path[0] != '/' && path[0] != '.') {
    return false;
  }

  if (!ValidationUtils::isPathSafe(path)) {
    return false;
  }

  size_t pos = 0;
  int dotdot_count = 0;
  while ((pos = path.find("../", pos)) != std::string::npos) {
    dotdot_count++;
    if (dotdot_count > 2) {
      return false;
    }
    pos += 3;
  }

  return true;
}
