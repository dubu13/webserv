#pragma once
#include <fstream>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <unistd.h>
#include <iostream>
#include <utility>

class LocationConfig {
  private:
    enum class LocationDirective {
      ROOT,
      INDEX,
      METHODS,
      AUTOINDEX,
      UPLOAD_STORE,
      UPLOAD_ENABLE,
      REDIRECTION,
      CGI_EXT,
      CGI_PATH,
      UNKNOWN
    };
    enum class CGIExtention {
      PY,
      UNHANDLED
    };

    std::unordered_map<std::string, LocationDirective> _locationDirectives;
    std::unordered_map<std::string, CGIExtention> _CGIExtensions;

  public:
    std::string root;
    std::string path;
    std::set<std::string> allowed_methods;
    std::optional<std::pair<int, std::string>> redirection;
    std::optional<std::string> index_file;
    std::optional<std::string> upload_path;
    std::optional<std::string> cgi_extension;
    std::optional<std::string> cgi_path;
    bool autoindex;
    bool allow_upload;
  
    LocationConfig();

    bool isMethodAllowed(const std::string &method) const;
    bool isCGI(const std::string &path) const;

    void parseLocationBlock(std::ifstream &file);
    void validateConfig() const;
    void validateUploadPath() const;
    void validateCGI() const;
};
