#include "Server.hpp"
#include "config/Config.hpp"
#include <csignal>
#include <iostream>

bool g_running = true;

void signalHandler(int signum) {
    (void)signum;
    g_running = false;
}

int main(int argc, char *argv[]) {
    try {
        signal(SIGINT, signalHandler);
        std::string configFile = (argc > 1) ? argv[1] : "config/default.conf";
        
        Config config(configFile);
        config.parseFromFile();
        
        const auto& servers = config.getServers();
        std::cout << "Loaded " << servers.size() << " server configurations" << std::endl;
        
        // Use first server configuration
        if (!servers.empty()) {
            const auto& [key, serverBlock] = *servers.begin();
            std::cout << "Using configuration:" << std::endl;
            std::cout << "  Host: " << serverBlock.host << std::endl;
            std::cout << "  Root: " << serverBlock.root << std::endl;
            std::cout << "  Max body size: " << serverBlock.clientMaxBodySize << std::endl;
            
            Server server(serverBlock);
            std::cout << "Starting webserver..." << std::endl;
            server.run();
        }
        
        std::cout << "Server stopped successfully" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
