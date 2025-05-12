#include "Server.hpp"
#include "HTTPResponse.hpp"
#include "HTTPGetRequest.hpp"
#include "IHTTPRequest.hpp"

#include <csignal>

bool g_running = true;

void signalHandler(int signum) {
    (void)signum; // Unused parameter
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
                server.acceptNewConnection();
            
                // if (server.getClientCount() > 0)
                    // std::cout << "Current clients: " << server.getClientCount() << std::endl;
                
                usleep(10000);
            }
            catch (const std::exception &e) {
                std::cerr << "Error: " << e.what() << std::endl;
                break;
            }
        }

        server.stop();
        std::cout << "Server stopped successfully" << std::endl;
    }
    catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
