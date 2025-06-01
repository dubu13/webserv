#include "server/MultiServerManager.hpp"
#include "utils/Logger.hpp"
#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <chrono>

extern std::atomic<bool> g_running;

MultiServerManager::~MultiServerManager() {
    if (_running.load()) {
        stopAll();
    }
}

void MultiServerManager::initializeServers(const Config& config) {
    Logger::info("Initializing multiple server instances...");
    
    const auto& servers = config.getServers();
    if (servers.empty()) {
        throw std::runtime_error("No server configurations found");
    }
    
    _servers.clear();
    _servers.reserve(servers.size());

    std::unordered_set<std::string> processedKeys;
    
    for (const auto& [key, serverBlock] : servers) {
        if (processedKeys.find(key) != processedKeys.end()) {
            Logger::debugf("Skipping duplicate server configuration for %s", key.c_str());
            continue;
        }
        processedKeys.insert(key);
        
        try {
            auto server = std::make_unique<Server>(serverBlock);
            Logger::infof("Created server instance for %s (Root: %s)", 
                         key.c_str(), serverBlock.root.c_str());
            Logger::infof("Server %s has %zu locations:", 
                         key.c_str(), serverBlock.locations.size());
            for (const auto& [path, location] : serverBlock.locations) {
                Logger::infof("  Location %s: root=%s, methods=%zu", 
                             path.c_str(), location.root.c_str(), location.allowedMethods.size());
            }
            _servers.push_back(std::move(server));
        } catch (const std::exception& e) {
            Logger::errorf("Failed to create server for %s: %s", key.c_str(), e.what());
            throw;
        }
    }
    
    setupServerMapping(config); 
    Logger::infof("Successfully initialized %zu server instances", _servers.size());
}

void MultiServerManager::setupServerMapping(const Config& config) {
    Logger::debug("Setting up server mapping for host:port routing...");
    
    _serverMap.clear();
    size_t serverIndex = 0;
    std::unordered_set<std::string> processedKeys;
    
    for (const auto& [key, serverBlock] : config.getServers()) {
        if (processedKeys.find(key) != processedKeys.end()) {
            continue;
        }
        processedKeys.insert(key);
        
        _serverMap[key] = serverIndex;
        Logger::debugf("Mapped %s -> server index %zu", key.c_str(), serverIndex);
        serverIndex++;
    }
    
    Logger::infof("Server mapping completed with %zu entries", _serverMap.size());
}

void MultiServerManager::startAll() {
    if (_running.load()) {
        Logger::warn("Servers are already running");
        return;
    }
    Logger::info("Starting all server instances...");
    _running.store(true); 
    _serverThreads.clear();
    _serverThreads.reserve(_servers.size());
    for (size_t i = 0; i < _servers.size(); ++i) {
        try {
            _serverThreads.emplace_back(&MultiServerManager::startServerThread, this, i);
            Logger::debugf("Started thread for server index %zu", i);
        } catch (const std::exception& e) {
            Logger::errorf("Failed to start thread for server index %zu: %s", i, e.what());
            stopAll();
            throw;
        }
    }  
    Logger::infof("All %zu server threads started successfully", _serverThreads.size());
}

void MultiServerManager::startServerThread(size_t serverIndex) {
    if (serverIndex >= _servers.size()) {
        Logger::errorf("Invalid server index: %zu", serverIndex);
        return;
    }
    try {
        Logger::infof("Starting server instance %zu", serverIndex);
        _servers[serverIndex]->run();
        Logger::infof("Server instance %zu stopped", serverIndex);
    } catch (const std::exception& e) {
        Logger::errorf("Server instance %zu crashed: %s", serverIndex, e.what());
    }
}

void MultiServerManager::stopAll() {
    if (!_running.load()) {
        Logger::debug("Servers are not running, nothing to stop");
        return;
    }
    Logger::info("Stopping all server instances...");
    _running.store(false);
    
    for (auto& server : _servers) {
        if (server) {
            server->stop();
        }
    }
    joinAll(); 
    Logger::info("All server instances stopped successfully");
}

void MultiServerManager::joinAll() {
    Logger::debug("Joining all server threads...");

    const int timeoutMs = 3000;
    auto startTime = std::chrono::steady_clock::now();
    
    for (auto& thread : _serverThreads) {
        if (thread.joinable()) {
            try {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime).count();
                if (elapsed >= timeoutMs) {
                    Logger::warn("Joining server thread timed out, forcefully detaching");
                    thread.detach();
                    continue;
                }
                thread.join();
                Logger::debug("Server thread joined successfully");
            } catch (const std::exception& e) {
                Logger::errorf("Error joining server thread: %s", e.what());
                if (thread.joinable()) {
                    thread.detach(); // Detach if join fails
                    Logger::warn("Forcefully detached server thread due to error");
                }
            }
        }
    }
    _serverThreads.clear();
    Logger::debug("All server threads joined");
}

const Server* MultiServerManager::getServer(const std::string& host, int port) const {
    std::string key = host + ":" + std::to_string(port);
    
    auto it = _serverMap.find(key);
    if (it != _serverMap.end() && it->second < _servers.size()) {
        return _servers[it->second].get();
    }
    
    Logger::debugf("No server found for %s", key.c_str());
    return nullptr;
}
