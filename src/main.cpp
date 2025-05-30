#include "Server.hpp"
#include "config/ServerConfig.hpp"
#include <csignal>
#include <fstream>
#include <iostream>
bool g_running = true;
void signalHandler(int signum) {
  (void)signum;
  g_running = false;
}
ServerConfig loadDefaultConfig(const std::string &configFile) {
  ServerConfig config;
  std::ifstream file(configFile);
  if (!file.is_open()) {
    std::cout << "Could not open config file: " << configFile
              << ", using default configuration" << std::endl;
    return config;
  }
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    if (line == "server {") {
      config.parseServerBlock(file);
      break;
    }
  }
  return config;
}
int main(int argc, char *argv[]) {
  try {
    signal(SIGINT, signalHandler);
    std::string configFile = (argc > 1) ? argv[1] : "config/default.conf";
    ServerConfig config = loadDefaultConfig(configFile);
    std::cout << "Using configuration:" << std::endl;
    std::cout << "  Host: " << config.host << std::endl;
    std::cout << "  Port: " << config.port << std::endl;
    std::cout << "  Root: " << config.root << std::endl;
    std::cout << "  Max body size: " << config.client_max_body_size
              << std::endl;
    Server server(config);
    std::cout << "Starting webserver..." << std::endl;
    server.run();
    std::cout << "Server stopped successfully" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
