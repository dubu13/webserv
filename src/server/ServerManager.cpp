#include "server/ServerManager.hpp"
#include "utils/Logger.hpp"
#include <unistd.h>
#include <stdexcept>
#include <sstream>
#include <atomic>  // Add this line

// Import global running flag from main.cpp
extern std::atomic<bool> g_running;

ServerManager::ServerManager() : _running(false) {}

ServerManager::~ServerManager() {
    stop();
}

void ServerManager::initializeServers(const Config& config) {
    const auto& serverConfigs = config.getServers();
    
    if (serverConfigs.empty())
        throw std::runtime_error("No server configurations found");
    
    for (const auto& [key, serverBlock] : serverConfigs) {
        try {
            auto server = std::make_unique<Server>(&serverBlock);
            _servers.push_back(std::move(server));

            // Map host:port combinations to server indices for virtual hosting
            for (const auto& listen : serverBlock.listenDirectives) {
                std::string host = listen.first.empty() ? "*" : listen.first;
                int port = listen.second;

                std::stringstream ss;
                ss << host << ":" << port;
                std::string hostPort = ss.str();

                // First server for a host:port is the default
                if (_hostPortMap.find(hostPort) == _hostPortMap.end())
                    _hostPortMap[hostPort] = _servers.size() - 1;
            }
        } catch (const std::exception& e) {
            Logger::logf<LogLevel::ERROR>("Failed to initialize server for " + key + ": " + e.what());
            throw;
        }
    }
    
    Logger::logf<LogLevel::INFO>("Initialized % servers", _servers.size());
}

void ServerManager::setupServerSockets() {
    for (size_t i = 0; i < _servers.size(); ++i) {
        int serverFd = _servers[i]->setupSocket();
        if (serverFd < 0) {
            throw std::runtime_error("Failed to setup socket for server " + std::to_string(i));
        }
        
        // Map server socket to its server index
        _socketToServerMap[serverFd] = i;
        
        // Add server socket to poller
        _poller.add(serverFd, POLLIN);
        
        Logger::logf<LogLevel::INFO>("Server % listening on fd %", i, serverFd);
    }
    Logger::logf<LogLevel::INFO>("All server sockets initialized");
}
bool ServerManager::start() {
    if (_running)
        return true;
        
    _running = false;  // Start not running
    
    try {
        setupServerSockets();
        
        _running = true;
        Logger::logf<LogLevel::INFO>("Starting all servers...");
        
        // Main server loop - checks both internal state and signal flag
        while (_running && g_running.load()) {
            processEvents(1000);  // 1 second timeout
            checkAllTimeouts();
        }
        
        Logger::logf<LogLevel::INFO>("Server manager stopped");
        return true;
    } catch (const std::exception& e) {
        Logger::logf<LogLevel::ERROR>("Error in server manager: %", e.what());
        return false;
    }
}

void ServerManager::stop() {
    _running = false;
    
    // Clean up all servers
    for (auto& server : _servers) {
        server->stop();
    }
    
    _socketToServerMap.clear();
}

bool ServerManager::processEvents(int timeout) {
    try {
        auto activeFds = _poller.poll(timeout);
        
        for (const auto& pfd : activeFds) {
            try {
                dispatchEvent(pfd);
            } catch (const std::exception& e) {
                Logger::logf<LogLevel::ERROR>("Error dispatching event: %", e.what());
                // Continue processing other events
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        if (errno == EINTR) {
            Logger::logf<LogLevel::WARN>("Poll interrupted by signal");
            return true;  // Just try again, don't report as error
        }
        
        Logger::logf<LogLevel::ERROR>("Error processing events: %", e.what());
        return false;
    }
}

void ServerManager::dispatchEvent(const struct pollfd& pfd) {
    // Check if it's a server socket (listening socket)
    auto serverIt = _socketToServerMap.find(pfd.fd);
    
    if (serverIt != _socketToServerMap.end()) {
        // It's a server socket, handle new connection
        size_t serverIndex = serverIt->second;
        int clientFd = _servers[serverIndex]->acceptConnection();
        
        // Add new client to poller
        if (clientFd > 0) {
            _poller.add(clientFd, POLLIN | POLLOUT);
        }
        return;
    }
    
    // It's a client socket, find which server it belongs to
    for (auto& server : _servers) {
        if (server->hasClient(pfd.fd)) {
            server->handleClient(pfd.fd);
            
            // Check if client was removed and clean up poller
            if (!server->hasClient(pfd.fd)) {
                _poller.remove(pfd.fd);
            }
            return;
        }
    }
    
    // If we got here, we don't know which server this fd belongs to
    Logger::logf<LogLevel::WARN>("Unhandled socket event for fd: %", pfd.fd);
    _poller.remove(pfd.fd);
    close(pfd.fd);
}

void ServerManager::checkAllTimeouts() {
    for (auto& server : _servers) {
        server->checkTimeouts();
    }
}