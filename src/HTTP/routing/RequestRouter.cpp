#include "HTTP/routing/RequestRouter.hpp"
#include "utils/Utils.hpp"
#include "utils/Logger.hpp"

RequestRouter::RequestRouter(const ServerBlock* config) : _config(config) {}

const LocationBlock* RequestRouter::findLocation(const std::string& uri) const {
    if (!_config) {
        Logger::debugf("No server configuration available");
        return nullptr;
    }

    Logger::debugf("Finding location for URI: %s", uri.c_str());

    std::string cleanUri = HttpUtils::sanitizePath(uri);
    Logger::debugf("Cleaned URI for location matching: %s", cleanUri.c_str());

    if (cleanUri == "/") {
        Logger::debugf("Root path detected, looking for exact '/' location match");
        auto it = _config->locations.find("/");
        if (it != _config->locations.end()) {
            Logger::debugf("Found exact match for root location '/' with root: %s",
                         it->second.root.c_str());
            return &it->second;
        }
        Logger::debugf("No exact match for root path, will use server root");
        return nullptr;
    }

    const LocationBlock* bestMatch = nullptr;
    size_t bestMatchLength = 0;

    Logger::debugf("Searching for best location match among %zu locations", _config->locations.size());

    for (const auto& [prefix, location] : _config->locations) {

        if (prefix == "/" && cleanUri != "/") {
            continue;
        }
        if (cleanUri.find(prefix) == 0) {
            if (prefix.length() > 1 &&
                cleanUri.length() > prefix.length() &&
                cleanUri[prefix.length()] != '/') {
                Logger::debugf("Skipping partial segment match: %s for URI: %s",
                             prefix.c_str(), cleanUri.c_str());
                continue;
            }

            if (prefix.length() > bestMatchLength) {
                bestMatch = &location;
                bestMatchLength = prefix.length();
                Logger::debugf("Found better match: %s (length: %zu)", prefix.c_str(), bestMatchLength);
            }
        }
    }

    if (bestMatch) {
        Logger::debugf("Best location match for '%s': '%s' with root: %s",
                     cleanUri.c_str(),
                     bestMatch->path.c_str(),
                     bestMatch->root.c_str());
    } else {
        Logger::debugf("No location match found for '%s', using server root", cleanUri.c_str());
    }

    return bestMatch;
}

std::string RequestRouter::resolveRoot(std::string_view uri, const LocationBlock* location) const {
    Logger::debugf("Resolving root for URI: %s", std::string(uri).c_str());

    if (location && !location->root.empty()) {
        Logger::debugf("Using location root: %s", location->root.c_str());
        return location->root;
    }

    if (_config && !_config->root.empty()) {
        Logger::debugf("Using server root: %s", _config->root.c_str());
        return _config->root;
    }

    Logger::debug("Using default root: ./www");
    return "./www";
}

bool RequestRouter::isMethodAllowed(const Request& request, const LocationBlock* location) const {
    if (!location || location->allowedMethods.empty()) {

        return true;
    }

    std::string methodStr = methodToString(request.requestLine.method);
    return location->allowedMethods.find(methodStr) != location->allowedMethods.end();
}
