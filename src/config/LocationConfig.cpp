#include "LocationConfig.hpp"
#include <iostream>
LocationConfig::LocationConfig()
    : root(), autoindex(false), allow_upload(false) {}
void LocationConfig::parseLocationBlock(std::ifstream &file) {
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    if (line.find('}') != std::string::npos)
      return;
    std::istringstream iss(line);
    std::string directive;
    iss >> directive;
    if (directive == "root") {
      iss >> root;
    } else if (directive == "index") {
      std::string value;
      iss >> value;
      index_file = value;
    } else if (directive == "methods") {
      std::string method;
      while (iss >> method)
        allowed_methods.insert(method);
    } else if (directive == "autoindex") {
      std::string value;
      iss >> value;
      autoindex = (value == "on" || value == "true");
    } else if (directive == "upload_store") {
      std::string value;
      iss >> value;
      upload_path = value;
    } else if (directive == "cgi_ext") {
      std::string value;
      iss >> value;
      cgi_extension = value;
    } else if (!directive.empty()) {
      std::cerr << "Warning: Unknown location directive: " << directive
                << std::endl;
    }
  }
}
