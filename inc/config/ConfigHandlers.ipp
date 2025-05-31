#pragma once
#include "config/ConfigUtils.hpp"

inline void Config::initializeHandlers() {
    initializeServerHandlers();
    initializeLocationHandlers();
}

inline void Config::initializeServerHandlers() {
    _serverHandlers = {
        {"listen", &Config::handleListen},
        {"host", &Config::handleHost},
        {"server_name", &Config::handleServerName},
        {"root", &Config::handleRoot},
        {"index", &Config::handleIndex},
        {"error_page", &Config::handleErrorPage},
        {"client_max_body_size", &Config::handleClientMaxBodySize}
    };
}

inline void Config::initializeLocationHandlers() {
    _locationHandlers = {
        {"root", &Config::handleLocationRoot},
        {"index", &Config::handleLocationIndex},
        {"methods", &Config::handleMethods},
        {"autoindex", &Config::handleAutoindex},
        {"upload_store", &Config::handleUploadStore},
        {"upload_enable", &Config::handleUploadEnable},
        {"return", &Config::handleReturn},
        {"cgi_ext", &Config::handleCgiExt},
        {"cgi_path", &Config::handleCgiPath},
        {"client_max_body_size", &Config::handleLocationClientMaxBodySize}
    };
}

inline void Config::handleListen(const std::string& value, ServerBlock& server) {
    auto [host, port] = ConfigUtils::parseListenDirective(value);
    server.listenDirectives.push_back({host, port});
}

inline void Config::handleHost(const std::string& value, ServerBlock& server) {
    if (!ConfigUtils::isValidIPv4(value)) {
        throw std::invalid_argument("Invalid host IP: " + value);
    }
    server.host = value;
}

inline void Config::handleServerName(const std::string& value, ServerBlock& server) {
    auto names = ConfigUtils::parseMultiValue(value);
    for (const auto& name : names) {
        if (!ConfigUtils::isValidServerName(name)) {
            throw std::invalid_argument("Invalid server name: " + name);
        }
        server.serverNames.push_back(name);
    }
}

inline void Config::handleRoot(const std::string& value, ServerBlock& server) {
    if (!ConfigUtils::isValidPath(value)) {
        throw std::invalid_argument("Invalid root path: " + value);
    }
    server.root = value;
}

inline void Config::handleIndex(const std::string& value, ServerBlock& server) {
    server.index = value;
}

inline void Config::handleErrorPage(const std::string& value, ServerBlock& server) {
    auto errorPages = ConfigUtils::parseErrorPages(value);
    server.errorPages.insert(errorPages.begin(), errorPages.end());
}

inline void Config::handleClientMaxBodySize(const std::string& value, ServerBlock& server) {
    server.clientMaxBodySize = ConfigUtils::parseSize(value);
}

inline void Config::handleLocationRoot(const std::string& value, LocationBlock& location) {
    if (!ConfigUtils::isValidPath(value)) {
        throw std::invalid_argument("Invalid location root: " + value);
    }
    location.root = value;
}

inline void Config::handleLocationIndex(const std::string& value, LocationBlock& location) {
    location.index = value;
}

inline void Config::handleMethods(const std::string& value, LocationBlock& location) {
    auto methods = ConfigUtils::parseMultiValue(value);
    location.allowedMethods.clear();
    for (const auto& method : methods) {
        if (!ConfigUtils::isValidMethod(method)) {
            throw std::invalid_argument("Invalid HTTP method: " + method);
        }
        location.allowedMethods.insert(method);
    }
}

inline void Config::handleAutoindex(const std::string& value, LocationBlock& location) {
    location.autoindex = (value == "on" || value == "true");
}

inline void Config::handleUploadStore(const std::string& value, LocationBlock& location) {
    location.uploadStore = value;
}

inline void Config::handleUploadEnable(const std::string& value, LocationBlock& location) {
    location.uploadEnable = (value == "on" || value == "true");
}

inline void Config::handleReturn(const std::string& value, LocationBlock& location) {
    location.redirection = value;
}

inline void Config::handleCgiExt(const std::string& value, LocationBlock& location) {
    location.cgiExtension = value;
}

inline void Config::handleCgiPath(const std::string& value, LocationBlock& location) {
    location.cgiPath = value;
}

inline void Config::handleLocationClientMaxBodySize(const std::string& value, LocationBlock& location) {
    location.clientMaxBodySize = ConfigUtils::parseSize(value);
}
