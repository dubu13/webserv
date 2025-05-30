#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <stdexcept>

// Forward declarations
struct LocationBlock;
struct ServerBlock;

// Include struct definitions
#include "config/LocationBlock.ipp"
#include "config/ServerBlock.ipp"

class Config {
private:
    std::string _fileName;
    std::unordered_map<std::string, ServerBlock> _servers;
    
    // Function pointer types for directive handlers
    using ServerDirectiveHandler = void (Config::*)(const std::string&, ServerBlock&);
    using LocationDirectiveHandler = void (Config::*)(const std::string&, LocationBlock&);
    
    // Directive handler maps
    std::unordered_map<std::string, ServerDirectiveHandler> _serverHandlers;
    std::unordered_map<std::string, LocationDirectiveHandler> _locationHandlers;
    
    // Handler initialization methods
    void initializeHandlers();
    void initializeServerHandlers();
    void initializeLocationHandlers();
    

    // Core parsing methods
    void parseServerBlock(const std::string& content, ServerBlock& server);
    void parseLocationBlock(const std::string& content, LocationBlock& location);

public:
    explicit Config(const std::string& fileName);
    
    // Main parsing interface
    void parseFromFile();
    
    // Access methods
    const std::unordered_map<std::string, ServerBlock>& getServers() const;
    const ServerBlock* getServer(const std::string& host, int port) const;

private:
    // Inline handler implementations
    void handleListen(const std::string& value, ServerBlock& server);
    void handleHost(const std::string& value, ServerBlock& server);
    void handleServerName(const std::string& value, ServerBlock& server);
    void handleRoot(const std::string& value, ServerBlock& server);
    void handleIndex(const std::string& value, ServerBlock& server);
    void handleErrorPage(const std::string& value, ServerBlock& server);
    void handleClientMaxBodySize(const std::string& value, ServerBlock& server);
    
    void handleLocationRoot(const std::string& value, LocationBlock& location);
    void handleLocationIndex(const std::string& value, LocationBlock& location);
    void handleMethods(const std::string& value, LocationBlock& location);
    void handleAutoindex(const std::string& value, LocationBlock& location);
    void handleUploadStore(const std::string& value, LocationBlock& location);
    void handleUploadEnable(const std::string& value, LocationBlock& location);
    void handleReturn(const std::string& value, LocationBlock& location);
    void handleCgiExt(const std::string& value, LocationBlock& location);
    void handleCgiPath(const std::string& value, LocationBlock& location);
};
