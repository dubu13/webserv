#include "config/Config.hpp"
#include "config/ConfigUtils.hpp"
#include "utils/Logger.hpp"
#include "utils/Utils.hpp"
#include <fstream>
#include <sstream>

Config::Config(const std::string &fileName) : _fileName(fileName) {
  Logger::logf<LogLevel::INFO>("Config constructor with file: %s",
                               fileName.c_str());
  initializeHandlers();
}

void Config::parseFromFile() {
  Logger::logf<LogLevel::INFO>("Starting to parse config file: %s",
                               _fileName.c_str());
  std::ifstream file(_fileName);
  if (!file.is_open()) {
    Logger::logf<LogLevel::ERROR>("Could not open config file: %s",
                                  _fileName.c_str());
    throw std::runtime_error("Could not open config file: " + _fileName);
  }

  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  file.close();

  std::vector<std::string> serverBlocks =
      ConfigUtils::extractServerBlocks(content);
  if (serverBlocks.empty()) {
    Logger::error("No server blocks found in config file");
    throw std::runtime_error("No server blocks found in config file");
  }

  Logger::logf<LogLevel::INFO>("Found %zu server blocks in config",
                               serverBlocks.size());

  for (size_t i = 0; i < serverBlocks.size(); ++i) {
    ServerBlock server;
    parseServerBlock(serverBlocks[i], server);

    for (const auto &listen : server.listenDirectives) {
      std::string hostToUse = server.host.empty() ? listen.first : server.host;
      std::string key = hostToUse + ":" + std::to_string(listen.second);
      _servers[key] = server;
      Logger::logf<LogLevel::INFO>("Added server configuration for %s",
                                   key.c_str());
    }
  }

  Logger::logf<LogLevel::INFO>(
      "Configuration parsing completed successfully. Total servers: %zu",
      _servers.size());
}

void Config::parseServerBlock(const std::string &content, ServerBlock &server) {
  std::istringstream iss(content);
  std::string line;

  while (std::getline(iss, line)) {
    auto [directive, value] = ConfigUtils::parseDirective(line);
    if (directive.empty())
      continue;

    if (directive == "location") {
      std::string locationPath = value;
      size_t bracePos = locationPath.find('{');
      if (bracePos != std::string::npos)
        locationPath = locationPath.substr(0, bracePos);
      locationPath = std::string(HttpUtils::trimWhitespace(locationPath));

      if (locationPath.empty() || !ConfigUtils::isValidPath(locationPath))
        throw std::invalid_argument("Invalid location path: " + locationPath);

      std::string locationContent = line + "\n";
      int braceCount = 1;
      while (std::getline(iss, line) && braceCount > 0) {
        for (char c : line) {
          if (c == '{')
            braceCount++;
          else if (c == '}')
            braceCount--;
        }
        if (braceCount > 0)
          locationContent += line + "\n";
      }
      LocationBlock location;
      location.path = locationPath;
      parseLocationBlock(locationContent, location);
      server.locations[locationPath] = location;
      continue;
    }
    auto it = _serverHandlers.find(directive);
    if (it != _serverHandlers.end()) {
      (this->*(it->second))(value, server);
    } else {
      Logger::logf<LogLevel::WARN>("Unknown server directive: %s",
                                   directive.c_str());
    }
  }

  if (server.listenDirectives.empty())
    throw std::invalid_argument(
        "Server block must have at least one listen directive");
}

void Config::parseLocationBlock(const std::string &content,
                                LocationBlock &location) {
  std::istringstream iss(content);
  std::string line;

  while (std::getline(iss, line)) {
    auto [directive, value] = ConfigUtils::parseDirective(line);
    if (directive.empty() || directive.find("location ") == 0)
      continue;
    auto it = _locationHandlers.find(directive);
    if (it != _locationHandlers.end())
      (this->*(it->second))(value, location);
  }
}

const std::unordered_map<std::string, ServerBlock> &Config::getServers() const {
  return _servers;
}

const ServerBlock *Config::getServer(const std::string &host, int port) const {
  std::string key = host + ":" + std::to_string(port);
  auto it = _servers.find(key);
  return (it != _servers.end()) ? &it->second : nullptr;
}
