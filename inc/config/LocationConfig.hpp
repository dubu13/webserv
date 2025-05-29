#pragma once
#include <fstream>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
class LocationConfig {
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
  void parseLocationBlock(std::ifstream &file);
};
