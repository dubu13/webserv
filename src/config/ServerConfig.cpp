#include <algorithm>
#include "ServerConfig.hpp"
ServerConfig::ServerConfig()
    : host("localhost"), port(8080), root("./www"),
      client_max_body_size(1024 * 1024) {
}
bool ServerConfig::matchesHost(const std::string &host) const {
  if (server_name.empty()) {
    return true;
  }
  return std::find(server_name.begin(), server_name.end(), host) !=
         server_name.end();
}
void ServerConfig::parseServerBlock(std::ifstream& file) {
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line == "}") break;
        std::istringstream iss(line);
        std::string directive;
        iss >> directive;
        if (directive == "listen") {
            iss >> port;
        } else if (directive == "server_name") {
            std::string name;
            while (iss >> name) {
                server_name.push_back(name);
            }
        } else if (directive == "root") {
            iss >> root;
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
        }
    }
}
const LocationConfig *ServerConfig::getLocation(const std::string &path) const {
    // Find the most specific location match
    const LocationConfig *best_match = nullptr;
    size_t best_match_length = 0;
    
    for (const auto &location_pair : locations) {
        const std::string &location_path = location_pair.first;
        if (path.find(location_path) == 0 && location_path.length() > best_match_length) {
            best_match = &location_pair.second;
            best_match_length = location_path.length();
        }
    }
    
    return best_match;
}