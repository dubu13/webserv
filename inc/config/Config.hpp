#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <stdexcept>

struct LocationBlock;
struct ServerBlock;

#include "config/LocationBlock.hpp"
#include "config/ServerBlock.hpp"

class Config {
private:
    std::string _fileName;
    std::unordered_map<std::string, ServerBlock> _servers;

    using ServerDirectiveHandler = void (Config::*)(const std::string&, ServerBlock&);
    using LocationDirectiveHandler = void (Config::*)(const std::string&, LocationBlock&);

    std::unordered_map<std::string, ServerDirectiveHandler> _serverHandlers;
    std::unordered_map<std::string, LocationDirectiveHandler> _locationHandlers;

    void initializeHandlers();
    void initializeServerHandlers();
    void initializeLocationHandlers();

    void parseServerBlock(const std::string& content, ServerBlock& server);
    void parseLocationBlock(const std::string& content, LocationBlock& location);

public:
    explicit Config(const std::string& fileName);

    void parseFromFile();

    const std::unordered_map<std::string, ServerBlock>& getServers() const;
    const ServerBlock* getServer(const std::string& host, int port) const;

private:

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
    void handleLocationClientMaxBodySize(const std::string& value, LocationBlock& location);
};
