#include "ServerConfig.hpp"

ServerConfig::ServerConfig() : _maxAllowedBodySize(1024 * 1024 * 1024) {
    _serverDirectives = {
        {"listen", ServerDirective::LISTEN},
        {"host", ServerDirective::HOST},
        {"server_name", ServerDirective::SERVER_NAME},
        {"root", ServerDirective::ROOT},
        {"index", ServerDirective::INDEX},
        {"error_pages", ServerDirective::ERROR_PAGES},
        {"client_max_body_size", ServerDirective::CLIENT_MAX_BODY_SIZE},
        {"location", ServerDirective::LOCATION}
    };
}


bool ServerConfig::matchesHost(const std::string &host) const {
    if (serverNames.empty())
        return true; // If no server names are specified, match all

    if (std::find(serverNames.begin(), serverNames.end(), host) != serverNames.end())
        return true;

    for (const auto& name : serverNames) {
        if (name == "*") // Wildcard match
            return true;

        if (name.size() > 2 && name[0] == '*' && name[1] == '.' && host.size() >= name.size() - 1) {
            std::string suffix = host.substr(host.size() - (name.size() - 1));
            if (suffix == name.substr(2)) // Match the suffix after the wildcard
                return true;
        }
    }
    return false;
}

const LocationConfig* ServerConfig::getLocation(const std::string &path) const {
    auto it = locations.find(path);
    if (it != locations.end())
        return &(it->second);
    
    std::string bestMatch = "";
    for (const auto &[locationPath, config] : locations) {
        if (path.find(locationPath) != 0)
            continue;
        if (locationPath.length() > bestMatch.length())
            bestMatch = locationPath;
    }

    if (!bestMatch.empty())
        return &(locations.at(bestMatch));

    it = locations.find("/");
    if (it != locations.end())
        return &(it->second);

    return nullptr;
}

void ServerConfig::parseServerBlock(std::ifstream &file) {
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        if (line == "}")
            return;

        if (!line.empty() && line.back() == ';')
            line.pop_back();

        std::istringstream iss(line);
        std::string directive;
        iss >> directive;

        if (iss.fail())
            throw std::runtime_error("Invalid directive format: " + line);

        auto it = _serverDirectives.find(directive);
        ServerDirective type = (it != _serverDirectives.end()) ? it->second : ServerDirective::UNKNOWN;

        switch (type) {
            case ServerDirective::LOCATION: {
                std::string locPath;
                iss >> locPath;
                if (iss.fail() || locPath.empty())
                    throw std::runtime_error("Missing value for location directive || Failed to parse");
                
                LocationConfig locConfig;
                locConfig.path = locPath;
                locConfig.parseLocationBlock(file);
                locations[locPath] = locConfig;
                break;
            }
            case ServerDirective::UNKNOWN:
                throw std::runtime_error("Unknown directive: " + directive);
            default:
                handleServerDirective(type, iss);
        }
        for (auto& [path, loc] : locations) {
            if (loc.root.empty())
                loc.root = this->root;
        }
    }
}

void ServerConfig::handleListen(std::string value) {
    size_t pos = value.find(':');
    if (pos != std::string::npos){
        try {
            std::string host = value.substr(0, pos);
            int port = std::stoi(value.substr(pos + 1));
            if (port < 1 || port > 65535)
                throw std::runtime_error("Invalid port : " + std::to_string(port) + " || Failed to parse");
            listenDirectives.emplace_back(host, port);
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to parse port number: " + value.substr(pos + 1) + " || " + e.what());
        }
    } else {
        try {
            int port = std::stoi(value);
            if (port < 1 || port > 65535)
                throw std::runtime_error("Invalid port number: " + std::to_string(port) + " || Failed to parse");
            listenDirectives.emplace_back("localhost", port);
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to parse port number: " + value + " || " + e.what());
        }
    }
}

size_t ServerConfig::handleBodySize(std::string& value) {
    try {
        char unit = value.back();
        size_t multiplier;
        size_t num = std::stoul(value.substr(0, value.size() - 1));
        
        if (std::isalpha(unit)) {
            switch (std::toupper(unit)) {
                case 'K': multiplier = 1024; break;
                case 'M': multiplier = 1024 * 1024; break;
                case 'G': multiplier = 1024 * 1024 * 1024; break;
                default:
                    throw std::runtime_error("Unknown size unit in client_max_body_size: " + sizeStr);
            }
        }
        if ((num * multiplier) > _maxAllowedBodySize || (num * multiplier) < 1)
            throw std::runtime_error("Invalid client body size. Maximum allowed size is " + std::to_string(_maxAllowedBodySize) + " bytes.");
        return num * multiplier;
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse client_max_body_size: " + value + " || " + e.what());
    }
}
void ServerConfig::handleServerDirective(ServerDirective type, std::istringstream& iss) {
    switch(type) {
        case ServerDirective::LISTEN: {
            std::string value;
            iss >> value;
            if (iss.fail() || value.empty())
                throw std::runtime_error("Missing value for listen directive || Failed to parse");
            handleListen(value);
            break;
        }
        case ServerDirective::HOST: {
            iss >> host;
            if ( iss.fail() || host.empty())
                throw std::runtime_error("Missing value for host directive || Failed to parse");
            break;
        }
        case ServerDirective::SERVER_NAME: {
            std::string name;
            while (iss >> name) {
                if (iss.fail() || name.empty())
                    throw std::runtime_error("Missing value for server_name directive || Failed to parse");
                serverNames.push_back(name);
            }
            break;
        }
        case ServerDirective::ROOT: {
            iss >> root;
            if (iss.fail() || root.empty())
                throw std::runtime_error("Missing value for root directive || Failed to parse");
            break;
        }
        case ServerDirective::INDEX: {
            std::string file;
            iss >> file;
            if (iss.fail() || file.empty())
                throw std::runtime_error("Missing value for index directive || Failed to parse");
            index_file = "/" + file;
            break;
        }
        case ServerDirective::ERROR_PAGES: {
            int error_code;
            std::string path;
            iss >> error_code;
            if (iss.fail() || error_code < 400 || error_code > 599)
                throw std::runtime_error("Invalid error code: " + std::to_string(error_code) + " || Failed to parse");
            iss >> path;
            if (iss.fail() || path.empty())
                throw std::runtime_error("Invalid error page path");
            error_pages[error_code] = path;
            break;
        }
        case ServerDirective::CLIENT_MAX_BODY_SIZE: {
            std::string value;
            iss >> value;
            if (iss.fail() || value.empty())
                throw std::runtime_error("Missing value for body size || Failed to parse");
            client_max_body_size = handleBodySize(value);
            break;      
        }
        default:
            throw std::runtime_error("Unknown directive");
    }
}

void ServerConfig::validateConfig() {
    if (listenDirectives.empty())
        throw std::runtime_error("No listen directives specified");
    if (!host.empty()) {
        for (auto &directive : listenDirectives)
            if (directive.first.empty())
                directive.first = host;
    }
    for (auto &directive : listenDirectives) {
        if (directive.first.empty() && host.empty())
            directive.first = "localhost";
        if (directive.first != "localhost" && !validateIPv4(directive.first))
            throw std::runtime_error("Invalid host: " + directive.first + ". Must be 'localhost' or a valid IPv4 address.");
    }
    if (root.empty())
        throw std::runtime_error("Root directory is not specified");
    if (!std::filesystem::exists(root))
        throw std::runtime_error("Root directory does not exist: " + root);
    if (!std::filesystem::is_directory(root))
        throw std::runtime_error("Root path is not a directory: " + root);
    
    for (const auto& [code, path] : error_pages) {
        std::string fullPath = root + path;
        if (!std::filesystem::exists(fullPath))
            throw std::runtime_error("Error page path does not exist: " + path);
    }
    if (index_file)
        if (!std::filesystem::exists(root + *index_file))
            std::cerr << "Warning! : Index file does not exist: " + root + *index_file << std::endl;
    if (!serverNames.empty())
        for (const auto& name : serverNames)
            if (!validateServerName(name))
                throw std::runtime_error("Invalid server name: " + name);
}

bool ServerConfig::validateIPv4(const std::string &ip) const {
    if (ip == "0.0.0.0")
        return true; // case for all interfaces

    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) == 1;
}

bool ServerConfig::validateServerName(const std::string& name) const {
    if (name.size() > 253)
        return false;

    if (validateIPv4(name) || isValidWildcard(name))
        return true;

    std::stringstream ss(name);
    std::string label;
    int labelCount = 0;

    while (std::getline(ss, label, '.')) {
        ++labelCount;
        if (!isLabelValid(label))
            return false;
    }
    return labelCount > 0;
}

bool ServerConfig::isValidWildcard(const std::string& name) const {
    if (name.find('*') == std::string::npos)
        return false;

    if (name == "*")
        return true;

    if (name.substr(0, 2) != "*.")
        return false;

    std::string rest = name.substr(2);
    std::stringstream ss(rest);
    std::string label;

    while (std::getline(ss, label, '.')) {
        if (!isLabelValid(label))
            return false;
    }
    return true;
}

bool ServerConfig::isLabelValid(const std::string& label) const {
    if (label.empty() || label.size() > 63)
        return false;
    if (label.front() == '-' || label.back() == '-')
        return false;
    for (char c : label) {
        if (!std::isalnum(c) && c != '-')
            return false;
    }
    return true;
}