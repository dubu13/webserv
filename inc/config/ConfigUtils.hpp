#pragma once

#include <string>
#include <vector>
#include <map>

namespace ConfigUtils {
    // String parsing utilities
    std::string trim(const std::string& str);
    std::vector<std::string> splitWhitespace(const std::string& str);
    
    // Block extraction utilities
    std::vector<std::string> extractServerBlocks(const std::string& content);
    
    // Directive parsing utilities
    std::pair<std::string, std::string> parseDirective(const std::string& line);
    std::vector<std::string> parseMultiValue(const std::string& value);
    
    // Validation utilities
    bool isValidIPv4(const std::string& ip);
    bool isValidMethod(const std::string& method);
    bool isValidServerName(const std::string& name);
    bool isValidPath(const std::string& path);
    
    // Value conversion utilities
    size_t parseSize(const std::string& value);
    std::pair<std::string, int> parseListenDirective(const std::string& value);
    std::map<int, std::string> parseErrorPages(const std::string& value);
}
