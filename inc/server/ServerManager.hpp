#pragma once

#include "Server.hpp"
#include "../config/Config.hpp"
#include <vector>
#include <map>
#include <memory>

class ServerManager {
private:
    std::vector<std::unique_ptr<Server>> _servers;
    std::map<std::string, size_t> _hostPortMap;
    std::map<int, size_t> _socketToServerMap; // Maps server socket FDs to server indices
    bool _running;
    Poller _poller;
    
public:
    ServerManager();
    ~ServerManager();
    
    void initializeServers(const Config& config);
    bool start(); // Main event loop
    void stop();
    
    size_t getServerCount() const { return _servers.size(); }

private:
    void setupServerSockets();
    bool processEvents(int timeout);
    void dispatchEvent(const struct pollfd& pfd);
    void checkAllTimeouts();
};