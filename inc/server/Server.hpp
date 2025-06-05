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

    bool start();
    void stop() { _running = false; }

private:
    void setupSocket();
    void acceptConnection();
    void handleClient(int fd);
    void removeClient(int fd);
    void checkTimeouts();
};
