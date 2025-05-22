#pragma once
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <stdexcept>

class LocationConfig {
  private:

      enum class LocationDirective {
        ROOT,
        INDEX,
        METHODS,
        AUTOINDEX,
        UPLOAD_STORE,
        CGI_EXT,
        UNKNOWN
    };

    std::unordered_map<std::string, LocationDirective> _locationDirectives;

  public:
    std::string root;
    std::string path;
    std::set<std::string> allowed_methods;
    std::optional<std::string> redirection;
    std::optional<std::string> index_file;
    std::optional<std::string> upload_path;
    std::optional<std::string> cgi_extension;
    bool autoindex{false};
    bool allow_upload{false};
  
    LocationConfig();

    // bool isMethodAllowed(const std::string &method) const;
    // bool isCGI(const std::string &path) const;

    void parseLocationBlock(std::ifstream &file);
};
