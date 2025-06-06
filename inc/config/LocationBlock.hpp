#pragma once
#include <set>
#include <string>

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

  LocationBlock()
      : autoindex(false), uploadEnable(false), clientMaxBodySize(0) {
    allowedMethods.insert("GET");
  }

  bool matchesPath(const std::string &requestPath) const {
    return requestPath.find(path) == 0;
  }
};
