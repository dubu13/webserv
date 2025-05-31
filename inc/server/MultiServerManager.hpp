#pragma once

#include "Server.hpp"
#include "config/Config.hpp"
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <string>

class MultiServerManager {
private:
    std::vector<std::unique_ptr<Server>> _servers;
    std::vector<std::thread> _serverThreads;
    std::atomic<bool> _running{false};

    std::unordered_map<std::string, size_t> _serverMap;
    
    void setupServerMapping(const Config& config);
    void startServerThread(size_t serverIndex);
    
public:
    explicit MultiServerManager() = default;
    ~MultiServerManager();
    MultiServerManager(const MultiServerManager&) = delete;
    MultiServerManager& operator=(const MultiServerManager&) = delete;
    MultiServerManager(MultiServerManager&&) = default;
    MultiServerManager& operator=(MultiServerManager&&) = default;
    void initializeServers(const Config& config);
    void startAll();
    void stopAll();
    void joinAll();
    size_t getServerCount() const { return _servers.size(); }
    bool isRunning() const { return _running.load(); }
    const Server* getServer(const std::string& host, int port) const;
};