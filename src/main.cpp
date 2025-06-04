#include <signal.h>
#include <iostream>
#include "config/Config.hpp"
#include "server/Server.hpp"
#include "utils/Logger.hpp"

static bool g_running = true;

void signalHandler(int) {
    g_running = false;
}

int main(int argc, char *argv[]) {
    try {

        signal(SIGINT, signalHandler);
        Logger::setLevel(LogLevel::INFO);

        std::string configFile = (argc > 1) ? argv[1] : "config/webserv.conf";
        Config config(configFile);
        config.parseFromFile();

        const auto& servers = config.getServers();
        if (servers.empty()) {
            std::cerr << "No server configurations found\n";
            return 1;
        }

        auto it = servers.begin();
        const ServerBlock* serverConfig = &it->second;

        Server server(serverConfig);
        std::cout << "Server starting on " << serverConfig->host << ":"
                 << (serverConfig->listenDirectives.empty() ? 8080 : serverConfig->listenDirectives[0].second) << "\n";

        while (g_running) {
            server.start();
        }

        std::cout << "Server stopped gracefully\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
