#include "LocationConfig.hpp"

LocationConfig::LocationConfig()
    : root(), autoindex(false), allow_upload(false) {

  _locationDirectives = {
      {"root", LocationDirective::ROOT},
      {"index", LocationDirective::INDEX},
      {"methods", LocationDirective::METHODS},
      {"autoindex", LocationDirective::AUTOINDEX},
      {"upload_store", LocationDirective::UPLOAD_STORE},
      {"cgi_ext", LocationDirective::CGI_EXT}};
}

// bool LocationConfig::isMethodAllowed(const std::string &method) const {
//   if (allowed_methods.empty()) {
//     return true; // If no methods are specified, allow all
//   }
//   return allowed_methods.find(method) != allowed_methods.end();
// }

// bool LocationConfig::isCGI(const std::string &path) const {
//   if (cgi_extension) {
//     return path.find(*cgi_extension) != std::string::npos;
//   }
//   return false;
// }


void LocationConfig::parseLocationBlock(std::ifstream &file) {
  std::string line;
  
  while (std::getline(file, line)) {
      if (line.empty() || line[0] == '#')
          continue;
      if (line == "}")
          return;

      std::istringstream iss(line);
      std::string directive;
      iss >> directive;

      auto it =_locationDirectives.find(directive);
      LocationDirective type = (it != _locationDirectives.end()) ? it->second : LocationDirective::UNKNOWN;

      std::string value;
      switch(type) {
          case LocationDirective::ROOT:
              iss >> root;
              break;
          case LocationDirective::INDEX:
              iss >> value;
              index_file = value;
              break;
          case LocationDirective::METHODS: {
              std::string method;
              while (iss >> method)
                  allowed_methods.insert(method);
              break;
          }
          case LocationDirective::AUTOINDEX: {
              iss >> value;
              autoindex = (value == "on" || value == "true");
              break;
          }
          case LocationDirective::UPLOAD_STORE:
              iss >> value;
              upload_path = value;
              break;
          case LocationDirective::CGI_EXT:
              iss >> value;
              cgi_extension = value;
              break;
          case LocationDirective::UNKNOWN :
              throw std::runtime_error("Unknown directive: " + directive);
      }
  }
}