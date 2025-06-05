#pragma once

#include <vector>
#include <string>
#include <map>
#include <ctime>
#include "ServerBlock.hpp"
#include "HTTP/routing/RequestRouter.hpp"
#include "HTTP/handlers/MethodDispatcher.hpp"
#include "Poller.hpp"

class Server {
private:
    int _serverFd;
    bool _running;
    Poller _poller;
    std::map<int, time_t> _clients;
    std::map<int, std::string> _clientBuffers;
    const ServerBlock* _config;
    RequestRouter _router;

public:
    explicit Server(const ServerBlock* config);
    ~Server();

    // Changed to return file descriptor
    int setupSocket();
    void stop() { _running = false; }
    
    // Methods that ServerManager will call
    void acceptConnection();
    void handleClient(int fd);
    void checkTimeouts();
    
    // Added for client lookup
    bool hasClient(int fd) const { return _clients.find(fd) != _clients.end(); }
    void closeClient(int fd) { removeClient(fd); }

private:
    void removeClient(int fd);
};
