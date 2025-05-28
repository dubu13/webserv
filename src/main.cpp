#include "Server.hpp"
#include "HTTPRequest.hpp"
#include <csignal>
#include <string>
#include <iostream>
bool g_running = true;
void signalHandler(int signum) {
    (void)signum;
    g_running = false;
}
int main() {
    try {
        signal(SIGINT, signalHandler);
        Server server(8080);
        std::cout << "Server started successfully" << std::endl;
        server.start();
        while (g_running) {
            try {
                std::vector<struct pollfd> active_fds = server.checkConnection();
                for (size_t i = 0; i < active_fds.size(); i++) {
                    struct pollfd& pfd = active_fds[i];
                    if (server.isServerSocket(pfd.fd)) {
                        if (server.hasActivity(&pfd, POLLIN)) {
                            server.acceptNewConnection();
                        }
                    } else {
                        if (server.hasActivity(&pfd, POLLIN)) {
                            server.handleClientRead(pfd.fd);
                        } else if (server.hasActivity(&pfd, POLLOUT)) {
                            server.handleClientWrite(pfd.fd);
                        }
                    }
                }
            }
            catch (const std::exception &e) {
                std::cerr << "Error in server loop: " << e.what() << std::endl;
            }
        }
        server.stop();
        std::cout << "Server stopped successfully" << std::endl;
    }
    catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
