#pragma once

#include "Socket.hpp"
#include "ConnectionManager.hpp"
#include "config/ServerConfig.hpp"  // Add ServerConfig header
#include <sys/resource.h> // for struct rlimit
#include <unordered_set>
#include <fcntl.h> // for fcntl
#include <unistd.h> // for close
#include <unordered_map>
#include <netinet/in.h> // for sockaddr_in

// Forward declaration to break circular dependency
class ClientHandler;
class Server {
private:
    Socket _socket;
    ConnectionManager _connectionManager;
    ClientHandler* _clientHandler;
    rlimit _rlim;
    bool _running;
    ServerConfig _config;  // Store server configuration

public:
    Server(const ServerConfig& config);  // Updated constructor
    ~Server();

    void start();
    void stop();
    void acceptNewConnection();
    void checkClientTimeouts();  // Added declaration
    
    int getSocketFd() const { return _socket.getFd(); }
    ConnectionManager& getConnectionManager() { return _connectionManager; }
    bool isServerSocket(int fd) const { return fd == _socket.getFd(); }
    
    // These methods now delegate to ClientHandler
    void handleClientRead(int clientFd);
    void handleClientWrite(int clientFd);
};
