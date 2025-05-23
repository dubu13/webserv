#include <algorithm>
#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
    : host("localhost"), port(8080), root("./www"),
      client_max_body_size(1024 * 1024) {

bool ServerConfig::matchesHost(const std::string &host) const {
  if (server_name.empty()) {
    return true; // If no server names are specified, match all
  }
  return std::find(server_name.begin(), server_name.end(), host) !=
         server_name.end();
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