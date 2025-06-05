#include <signal.h>
#include <iostream>
#include "config/Config.hpp"
#include "server/ServerManager.hpp"
#include "utils/Logger.hpp"
#include <atomic>

std::atomic<bool> g_running = true;

void signalHandler(int signum) {
    const char* signame = "";
    switch (signum) {
        case SIGINT: signame = "SIGINT"; break;
        case SIGTERM: signame = "SIGTERM"; break;
        case SIGQUIT: signame = "SIGQUIT"; break;
        default: signame = "Unknown"; break;
    }
    std::cerr << "\nReceived signal " << signame << " (" << signum << "), shutting down...\n";
    g_running = false;
}

int main(int argc, char *argv[]) {
    try {
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        signal(SIGQUIT, signalHandler);
        signal(SIGPIPE, SIG_IGN);
 
        Logger::setLevel(LogLevel::INFO);

        std::string configFile = (argc > 1) ? argv[1] : "config/webserv.conf";
        Config config(configFile);
        config.parseFromFile();

        ServerManager server;
        server.initializeServers(config);
        
        std::cout << "Starting " << server.getServerCount() << " server(s)...\n";
        
        if (!server.start()) {
            std::cerr << "Server start failed!\n";
            return 1;
        }
        
        std::cout << "Server shutdown complete.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
