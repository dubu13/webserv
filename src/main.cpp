#include "Server.hpp"
#include "config/Config.hpp"
#include "utils/Logger.hpp"
#include <csignal>
#include <iostream>

bool g_running = true;

void signalHandler(int signum) {
    Logger::infof("Received signal %d, shutting down gracefully", signum);
    g_running = false;
}

int main(int argc, char *argv[]) {
    try {
        // Initialize logging
        Logger::setLevel(LogLevel::DEBUG);
        Logger::enableFileLogging("webserv.log");
        Logger::info("WebServer starting up...");
        
        signal(SIGINT, signalHandler);
        std::string configFile = (argc > 1) ? argv[1] : "config/default.conf";
        Logger::infof("Using config file: %s", configFile.c_str());
        
        Config config(configFile);
        config.parseFromFile();
        
        const auto& servers = config.getServers();
        Logger::infof("Loaded %zu server configurations", servers.size());
        
        // Use first server configuration
        if (!servers.empty()) {
            const auto& [key, serverBlock] = *servers.begin();
            Logger::infof("Using server configuration - Host: %s, Root: %s", 
                         serverBlock.host.c_str(), serverBlock.root.c_str());
            // Debug: print all locations
            Logger::infof("Server has %zu locations:", serverBlock.locations.size());
            for (const auto& [path, location] : serverBlock.locations) {
                Logger::infof("  Location %s: root=%s", path.c_str(), location.root.c_str());
            }
            Server server(serverBlock);
            Logger::info("Server instance created, starting run loop");
            server.run();
        }
        Logger::info("Server stopped successfully");
    } catch (const std::exception &e) {
        Logger::errorf("Fatal error: %s", e.what());
        return 1;
    }
    return 0;
}
