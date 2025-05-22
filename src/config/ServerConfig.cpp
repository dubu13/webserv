#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
    : host("localhost"), port(8080), root("./www"),
      client_max_body_size(1024 * 1024) {

        _serverDirectives = {
          {"port", ServerDirective::PORT},
          {"host", ServerDirective::HOST},
          {"server_name", ServerDirective::SERVER_NAME},
          {"root", ServerDirective::ROOT},
          {"error_pages", ServerDirective::ERROR_PAGES},
          {"client_max_body_size", ServerDirective::CLIENT_MAX_BODY_SIZE},
          {"location", ServerDirective::LOCATION}
      };
} // 1 MB default

// bool ServerConfig::matchesHost(const std::string &host) const {
//   if (server_name.empty()) {
//     return true; // If no server names are specified, match all
//   }
//   return std::find(server_names.begin(), server_names.end(), host) !=
//          server_names.end();
// }

// const LocationConfig *ServerConfig::getLocation(const std::string &path) const {
//   for (const auto &location : locations) {
//     if (path.find(location.first) == 0) {
//       return &location.second;
//     }
//   }
//   return nullptr; // No matching location found
// }


void ServerConfig::parseServerBlock(std::ifstream &file) {
  std::string line;

  while (std::getline(file, line)) {
      if (line.empty() || line[0] == '#')
          continue;
      if (line == "}")
          return;
       std::istringstream iss(line);
       std::string directive;
       iss >> directive;

       auto it = _serverDirectives.find(directive);
       ServerDirective type = (it != _serverDirectives.end()) ? it->second : ServerDirective::UNKNOWN;

       switch(type) {
          case ServerDirective::LOCATION: {
              std::string path;
              iss >> path;
              LocationConfig locationBlock;
              locationBlock.parseLocationBlock(file);
              locations[path] = locationBlock;
              break;
          }
          case ServerDirective::UNKNOWN:
              throw std::runtime_error("Unknown directive: " + directive);
          default:
              handleServerDirective(type, iss);
       }
  }
}

void ServerConfig::handleServerDirective(ServerDirective type, std::istringstream& iss) {
  std::string value;

  switch(type) {
      case ServerDirective::PORT:
          iss >> port;
          break;
      case ServerDirective::HOST:
          iss >> host;
          break;
      case ServerDirective::SERVER_NAME:
          iss >> server_name;
          break;
      case ServerDirective::ROOT:
          iss >> root;
          break;
      case ServerDirective::ERROR_PAGES: {
          int error_code;
          std::string path;
          iss >> error_code >> path;
          error_pages[error_code] = path;
          break;
      }
      case ServerDirective::CLIENT_MAX_BODY_SIZE:
          iss >> client_max_body_size;
          break;
      default:
          throw std::runtime_error("Unknown directive");
  }
}