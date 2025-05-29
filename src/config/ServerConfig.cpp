#include "ServerConfig.hpp"
#include <algorithm>
#include <iostream>
ServerConfig::ServerConfig()
    : host("localhost"), port(8080), root("./www"),
      client_max_body_size(1024 * 1024) {}
bool ServerConfig::matchesHost(const std::string &host) const {
  if (server_name.empty()) {
    return true;
  }
  return std::find(server_name.begin(), server_name.end(), host) !=
         server_name.end();
}
void ServerConfig::parseServerBlock(std::ifstream &file) {
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    if (line == "}")
      break;
    std::istringstream iss(line);
    std::string directive;
    iss >> directive;
    if (directive == "listen") {
      std::string listen_value;
      iss >> listen_value;
      size_t colon_pos = listen_value.find(':');
      if (colon_pos != std::string::npos) {
        host = listen_value.substr(0, colon_pos);
        port = std::stoi(listen_value.substr(colon_pos + 1));
      } else {
        port = std::stoi(listen_value);
      }
    } else if (directive == "host") {
      std::string host_value;
      iss >> host_value;
      if (!host_value.empty() && host_value.back() == ';') {
        host_value.pop_back();
      }
      host = host_value;
    } else if (directive == "server_name") {
      std::string name;
      while (iss >> name) {
        server_name.push_back(name);
      }
    } else if (directive == "root") {
      std::string root_value;
      iss >> root_value;
      if (!root_value.empty() && root_value.back() == ';') {
        root_value.pop_back();
      }
      root = root_value;
    } else if (directive == "client_max_body_size") {
      iss >> client_max_body_size;
    } else if (directive == "error_page") {
      int code;
      std::string path;
      iss >> code >> path;
      error_pages[code] = path;
    } else if (directive == "location") {
      std::string path;
      iss >> path;
      LocationConfig location;
      location.path = path;
      location.parseLocationBlock(file);
      locations[path] = location;
    } else if (!directive.empty()) {
      std::cerr << "Warning: Unknown server directive: " << directive
                << std::endl;
    }
  }
}
const LocationConfig *ServerConfig::getLocation(const std::string &path) const {
  const LocationConfig *best_match = nullptr;
  size_t best_match_length = 0;
  for (const auto &location_pair : locations) {
    const std::string &location_path = location_pair.first;
    if (path.find(location_path) == 0 &&
        location_path.length() > best_match_length) {
      best_match = &location_pair.second;
      best_match_length = location_path.length();
    }
  }
  return best_match;
}
