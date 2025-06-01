#include "server/MultiServerManager.hpp"
#include "config/Config.hpp"
#include "utils/Logger.hpp"
#include <csignal>
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>

std::atomic<bool> g_running{true};

void signalHandler(int signum) {
    Logger::infof("Received signal %d, shutting down gracefully", signum);
    g_running.store(false);
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
        
        // Stage 1.2: Multiple Server Support - Use MultiServerManager
        if (!servers.empty()) {
            MultiServerManager serverManager;
            
            // Initialize all server instances from configuration
            serverManager.initializeServers(config);
            Logger::infof("Initialized %zu server instances", serverManager.getServerCount());
            
            // Start all servers concurrently
            serverManager.startAll();
            Logger::info("All servers started, running until shutdown signal");
            
            // Wait for shutdown signal
            while (g_running.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            // Graceful shutdown
            Logger::info("Shutdown signal received, stopping all servers");
            serverManager.stopAll();
        } else {
            Logger::error("No server configurations found in config file");
            return 1;
        }
        Logger::info("Server stopped successfully");
    } catch (const std::exception &e) {
        Logger::errorf("Fatal error: %s", e.what());
        return 1;
    }
    return 0;
}
