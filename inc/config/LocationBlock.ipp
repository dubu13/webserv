#pragma once

struct LocationBlock {
    std::string path;
    std::string root;
    std::string index;
    std::set<std::string> allowedMethods;
    bool autoindex;
    std::string uploadStore;
    bool uploadEnable;
    std::string redirection;
    std::string cgiExtension;
    std::string cgiPath;
    size_t clientMaxBodySize;
    
    LocationBlock();
    bool matchesPath(const std::string& requestPath) const;
};

inline LocationBlock::LocationBlock() 
    : autoindex(false), uploadEnable(false), clientMaxBodySize(0) {
    allowedMethods.insert("GET");
}

inline bool LocationBlock::matchesPath(const std::string& requestPath) const {
    return requestPath.find(path) == 0;
}
