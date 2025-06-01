#include "config/Config.hpp"

LocationBlock::LocationBlock() 
    : autoindex(false), uploadEnable(false), clientMaxBodySize(0) {
    allowedMethods.insert("GET");
}

bool LocationBlock::matchesPath(const std::string& requestPath) const {
    return requestPath.find(path) == 0;
}
