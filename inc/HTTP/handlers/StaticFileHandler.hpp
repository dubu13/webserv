#pragma once
#include <string>
#include <string_view>
#include <memory>
#include "utils/Utils.hpp"

class StaticFileHandler {
public:

    static std::string handleRequest(std::string_view root, std::string_view uri);

private:

    static std::string serveFile(std::string_view filePath);
    static std::string serveDirectory(std::string_view dirPath, std::string_view requestUri);
    static std::string findIndexFile(std::string_view dirPath);
    static std::string generateWelcomePage();
};
