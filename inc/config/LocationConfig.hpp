#pragma once
#include <optional>
#include <set>
#include <string>

class LocationConfig {
public:
  std::set<std::string> allowed_methods;
  std::optional<std::string> redirection;
  std::string root;
  std::optional<std::string> index_file;
  std::optional<std::string> upload_path;
  std::optional<std::string> cgi_extension;
  bool autoindex;
  bool allow_upload;

  LocationConfig();

  bool isMethodAllowed(const std::string &method) const;
  bool isCGI(const std::string &path) const;
};
