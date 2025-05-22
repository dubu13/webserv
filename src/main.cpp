#include "Server.hpp"
#include "HTTPGetRequest.hpp"
#include "IHTTPRequest.hpp"
#include "ConnectionManager.hpp"
#include "Socket.hpp" 
#include <csignal>
#include <string>
#include <iostream>

bool g_running = true;

void signalHandler(int signum) {
    (void)signum; // Unused parameter
    g_running = false;
}

int main() {
    try {
        signal(SIGINT, signalHandler);

        Server server(8080);  // Initialize with port
        std::cout << "Server started successfully" << std::endl;
        server.start();
        
        // Use the server's connection manager
        ConnectionManager& connectionManager = server.getConnectionManager();

        // Add server socket to poll fds
        connectionManager.addConnection(server.getSocketFd(), POLLIN);

        while (g_running) {
            try {
                // Check for active connections
                std::vector<struct pollfd> active_fds = connectionManager.checkConnection();
                
                // Process active connections
                for (size_t i = 0; i < active_fds.size(); i++) {
                    struct pollfd& pfd = active_fds[i]; // Use reference to non-const
                    
                    if (server.isServerSocket(pfd.fd)) {
                        // Server socket is active, accept new connection
                        if (connectionManager.hasActivity(&pfd, POLLIN)) {
                            server.acceptNewConnection();
                        }
                    } else {
                        // Client socket is active
                        if (connectionManager.hasActivity(&pfd, POLLIN)) {
                            // Client has data to read
                            server.handleClientRead(pfd.fd);
                        } else if (connectionManager.hasActivity(&pfd, POLLOUT)) {
                            // Client is ready for writing
                            server.handleClientWrite(pfd.fd);
                        }
                    }
                }
                
                usleep(10000); // Small delay to prevent CPU hogging
            }
            catch (const SocketException& e) {
                std::cerr << "Socket error: " << e.what() << std::endl;
            }
            catch (const HTTPParseException& e) {
                std::cerr << "HTTP parsing error: " << e.what() << std::endl;
            }
            catch (const std::exception &e) {
                std::cerr << "Error in server loop: " << e.what() << std::endl;
            }
        }

        server.stop();
        std::cout << "Server stopped successfully" << std::endl;
    }
    catch (const SocketException& e) {
        std::cerr << "Fatal socket error: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
