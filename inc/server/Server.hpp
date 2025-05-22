#pragma once

#include "Socket.hpp"
#include "ConnectionManager.hpp"
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

public:
    Server(int port = 8080);
    ~Server();

    void start();
    void stop();
    void acceptNewConnection();
    
    int getSocketFd() const { return _socket.getFd(); }
    ConnectionManager& getConnectionManager() { return _connectionManager; }
    bool isServerSocket(int fd) const { return fd == _socket.getFd(); }
    
    // These methods now delegate to ClientHandler
    void handleClientRead(int clientFd);
    void handleClientWrite(int clientFd);
};
