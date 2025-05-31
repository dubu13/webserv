#include "config/ConfigUtils.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <arpa/inet.h>

namespace ConfigUtils {

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> splitWhitespace(const std::string& str) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<std::string> extractServerBlocks(const std::string& content) {
    std::vector<std::string> blocks;
    std::istringstream iss(content);
    std::string line;
    
    while (std::getline(iss, line)) {
        line = trim(line);
        if (line == "server {") {
            std::string blockContent;
            int braceCount = 1;
            
            while (std::getline(iss, line) && braceCount > 0) {
                for (char c : line) {
                    if (c == '{') braceCount++;
                    else if (c == '}') braceCount--;
                }
                if (braceCount > 0) {
                    blockContent += line + "\n";
                }
            }
            blocks.push_back(blockContent);
        }
    }
    return blocks;
}

std::pair<std::string, std::string> parseDirective(const std::string& line) {
    std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed[0] == '#') {
        return {"", ""};
    }
    
    std::istringstream iss(trimmed);
    std::string directive;
    if (!(iss >> directive)) {
        return {"", ""};
    }
    
    std::string value;
    std::getline(iss, value);
    value = trim(value);

    if (!value.empty() && value.back() == ';') {
        value.pop_back();
        value = trim(value);
    }
    
    return {directive, value};
}

std::vector<std::string> parseMultiValue(const std::string& value) {
    return splitWhitespace(value);
}


size_t parseSize(const std::string& value) {
    if (value.empty()) {
        throw std::invalid_argument("Empty size value");
    }
    
    std::string numStr = value;
    size_t multiplier = 1;
    
    char last = std::tolower(value.back());
    if (last == 'k') {
        multiplier = 1024;
        numStr.pop_back();
    } else if (last == 'm') {
        multiplier = 1024 * 1024;
        numStr.pop_back();
    } else if (last == 'g') {
        multiplier = 1024 * 1024 * 1024;
        numStr.pop_back();
    }
    
    try {
        size_t num = std::stoull(numStr);
        return num * multiplier;
    } catch (const std::exception&) {
        throw std::invalid_argument("Invalid size format");
    }
}

std::pair<std::string, int> parseListenDirective(const std::string& value) {
    std::istringstream iss(value);
    std::string token;
    
    if (!(iss >> token)) {
        throw std::invalid_argument("Empty listen directive");
    }
    
    size_t colonPos = token.find(':');
    if (colonPos == std::string::npos) {
        try {
            int port = std::stoi(token);
            if (port < 1 || port > 65535) {
                throw std::invalid_argument("Invalid port range");
            }
            return {"0.0.0.0", port};
        } catch (const std::exception&) {
            throw std::invalid_argument("Invalid port format");
        }
    }
    
    std::string host = token.substr(0, colonPos);
    std::string portStr = token.substr(colonPos + 1);
    
    if (host.empty()) host = "0.0.0.0";
    if (!isValidIPv4(host)) {
        throw std::invalid_argument("Invalid IP address");
    }
    
    try {
        int port = std::stoi(portStr);
        if (port < 1 || port > 65535) {
            throw std::invalid_argument("Invalid port range");
        }
        return {host, port};
    } catch (const std::exception&) {
        throw std::invalid_argument("Invalid port format");
    }
}

std::map<int, std::string> parseErrorPages(const std::string& value) {
    std::map<int, std::string> errorPages;
    std::istringstream iss(value);
    std::vector<std::string> codes;
    std::string token;
    
    while (iss >> token) {
        codes.push_back(token);
    }
    
    if (codes.size() < 2) {
        throw std::invalid_argument("Invalid error_page format");
    }
    
    std::string path = codes.back();
    for (size_t i = 0; i < codes.size() - 1; ++i) {
        try {
            int code = std::stoi(codes[i]);
            if (code < 100 || code > 599) {
                throw std::invalid_argument("Invalid HTTP status code");
            }
            errorPages[code] = path;
        } catch (const std::exception&) {
            throw std::invalid_argument("Invalid HTTP status code format");
        }
    }
    
    return errorPages;
}

bool isValidIPv4(const std::string& ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) != 0;
}

bool isValidMethod(const std::string& method) {
    static const std::vector<std::string> validMethods = {
        "GET", "POST", "DELETE", "PUT", "HEAD", "OPTIONS", "PATCH"
    };
    return std::find(validMethods.begin(), validMethods.end(), method) != validMethods.end();
}

bool isValidServerName(const std::string& name) {
    if (name.empty()) return false;
    if (name == "*") return true;
    
    // Check for wildcard pattern *.domain.com
    if (name.size() > 2 && name.substr(0, 2) == "*.") {
        return isValidServerName(name.substr(2));
    }
    
    // Basic domain name validation
    for (char c : name) {
        if (!std::isalnum(c) && c != '.' && c != '-') {
            return false;
        }
    }
    return true;
}

bool isValidPath(const std::string& path) {
    return !path.empty() && (path[0] == '/' || path[0] == '.');
}

}
