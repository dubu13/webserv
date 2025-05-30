#pragma once

// LocationBlock struct definition and implementation
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
    
    LocationBlock();
    bool matchesPath(const std::string& requestPath) const;
};

// LocationBlock implementation
inline LocationBlock::LocationBlock() 
    : autoindex(false), uploadEnable(false) {
    allowedMethods.insert("GET");
}

inline bool LocationBlock::matchesPath(const std::string& requestPath) const {
    return requestPath.find(path) == 0;
}
