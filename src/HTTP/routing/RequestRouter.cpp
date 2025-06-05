#include "HTTP/routing/RequestRouter.hpp"
#include "utils/Utils.hpp"
#include "utils/Logger.hpp"

RequestRouter::RequestRouter(const ServerBlock* config) : _config(config) {}

const LocationBlock* RequestRouter::findLocation(const std::string& uri) const {
    if (!_config) {
        return nullptr;
    }

    return _config->getLocation(uri);
}

std::string RequestRouter::resolveRoot(const LocationBlock* location) const {

    if (location && !location->root.empty()) {
        return location->root;
    }

    if (_config && !_config->root.empty()) {
        return _config->root;
    }

    return "./www";
}

bool RequestRouter::isMethodAllowed(const Request& request, const LocationBlock* location) const {
    if (!location || location->allowedMethods.empty()) {
        return true;
    }

    std::string methodStr = methodToString(request.requestLine.method);
    return location->allowedMethods.find(methodStr) != location->allowedMethods.end();
}

bool RequestRouter::hasRedirection(const LocationBlock* location) const {
    return location && !location->redirection.empty();
}

std::string RequestRouter::getRedirectionTarget(const LocationBlock* location) const {
    if (!location || location->redirection.empty()) {
        return "";
    }

    return location->redirection;
}
std::string RequestRouter::handleRedirection(const LocationBlock* location) const {
    if (!hasRedirection(location))
        return "";

    std::string target = getRedirectionTarget(location);
    int code = 302;
    std::string redirectUrl = target;

    size_t spacePos = target.find(' ');
    if (spacePos != std::string::npos) {
        std::string codeStr = target.substr(0, spacePos);

        try {
            int parsedCode = std::stoi(codeStr);

            if (parsedCode == 301 || parsedCode == 302 ||
                parsedCode == 303 || parsedCode == 307 || parsedCode == 308) {
                code = parsedCode;
                redirectUrl = target.substr(spacePos + 1);
            }
        } catch (const std::exception& e) {
        }
    }

    return HttpResponse::redirect(redirectUrl, code).str();
}
