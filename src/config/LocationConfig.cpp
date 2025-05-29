#include "LocationConfig.hpp"
#include <iostream>
LocationConfig::LocationConfig()
    : root(), autoindex(false), allow_upload(false) {

    _locationDirectives = {
        {"root", LocationDirective::ROOT},
        {"index", LocationDirective::INDEX},
        {"methods", LocationDirective::METHODS},
        {"autoindex", LocationDirective::AUTOINDEX},
        {"upload_store", LocationDirective::UPLOAD_STORE},
        {"upload_enable", LocationDirective::UPLOAD_ENABLE},
        {"return", LocationDirective::REDIRECTION},
        {"cgi_extension", LocationDirective::CGI_EXT},
        {"cgi_path", LocationDirective::CGI_PATH}
    };
    _CGIExtensions = {
        {".py", CGIExtention::PY}
    };
}

bool LocationConfig::isMethodAllowed(const std::string &method) const {
  if (allowed_methods.empty())
        return true;
  return allowed_methods.find(method) != allowed_methods.end();
}

bool LocationConfig::isCGI(const std::string &path) const {
  if (cgi_extension)
        return path.find(*cgi_extension) != std::string::npos;
  return false;
}

void LocationConfig::parseLocationBlock(std::ifstream &file) {
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#')
         continue;
    if (line == "}")
         return;
    if (!line.empty() && line.back() == ';')
         line.pop_back();

    std::istringstream iss(line);
    std::string directive;
    iss >> directive;


    if (iss.fail())
        throw std::runtime_error("Invalid directive format: " + line);

    auto it =_locationDirectives.find(directive);
    LocationDirective type = (it != _locationDirectives.end()) ? it->second : LocationDirective::UNKNOWN;

    std::string value;
    switch(type) {
        case LocationDirective::ROOT:
            iss >> root;
            if (iss.fail() || root.empty())
                throw std::runtime_error("Missing value for root directive || Failed to parse");
            break;
        case LocationDirective::INDEX:
            iss >> value;
            if (iss.fail() || value.empty())
                throw std::runtime_error("Missing value for index directive || Failed to parse");
            index_file = "/" + value;
            break;
        case LocationDirective::METHODS: {
            std::string method;
            while (iss >> method) {
            if (iss.fail() || method.empty())
                throw std::runtime_error("Missing value for method directive || Failed to parse");
            if (method != "GET" && method != "POST" && method != "DELETE")
                throw std::runtime_error("Invalid method: " + method);
            allowed_methods.insert(method);
            }
            break;
        }
        case LocationDirective::AUTOINDEX: {
            iss >> value;
            if (iss.fail() || value.empty())
                throw std::runtime_error("Missing value for autoindex directive || Failed to parse");
            if (value != "on" && value != "off")
                throw std::runtime_error("Invalid value for autoindex directive: " + value);
            autoindex = (value == "on");
            break;
        }
        case LocationDirective::UPLOAD_STORE:
            iss >> value;
            if (iss.fail() || value.empty())
                throw std::runtime_error("Missing value for upload_store directive || Failed to parse");
            upload_path = value;
            break;
        case LocationDirective::UPLOAD_ENABLE:
            iss >> value;
            if (iss.fail() || value.empty())
                throw std::runtime_error("Missing value for upload_enable directive || Failed to parse");
            if (value != "on" && value != "off")
                throw std::runtime_error("Invalid value for upload_enable directive: " + value);
            allow_upload = (value == "on");
            break;
        case LocationDirective::CGI_EXT:
            iss >> value;
            if (iss.fail() || value.empty())
                throw std::runtime_error("Missing value for cgi_ext directive || Failed to parse");
            cgi_extension = value;
            break;
        case LocationDirective::CGI_PATH:
            iss >> value;
            if (iss.fail() || value.empty())
                throw std::runtime_error("Missing value for cgi_path directive || Failed to parse");
            cgi_path = value;
            break;
        case LocationDirective::REDIRECTION:
            int status_code;
            iss >> status_code >> value;
            if (iss.fail() || value.empty())
                throw std::runtime_error("Missing value for redirection directive || Failed to parse");
            if (status_code < 300 || status_code > 399)
                throw std::runtime_error("Invalid status code for redirection: " + std::to_string(status_code));
            redirection = std::make_pair(status_code, value);
            break;
        case LocationDirective::UNKNOWN :
            throw std::runtime_error("Unknown directive: " + directive);
        }
    }
}

void LocationConfig::validateConfig() const {
    if (!root.empty()) {
        if (!std::filesystem::exists(root))
            throw std::runtime_error("Root directory does not exist: " + root);
        if (!std::filesystem::is_directory(root))
            throw std::runtime_error("Root path is not a directory: " + root);
    }
    if (upload_path)
        validateUploadPath();
    if (bool(cgi_extension) != bool(cgi_path))
        throw std::runtime_error("CGI extension and CGI path must be both set or both unset");
    if (cgi_extension)
        validateCGI();
    if (!autoindex && index_file)
        if (!std::filesystem::exists(root + *index_file))
            std::cerr << "WarninIndex file does not exist: " + (root + *index_file)
                    << "autoindex is off, this will result in 404 errors!" << std::endl;
}

void LocationConfig::validateUploadPath() const {
    if (!std::filesystem::exists(*upload_path)) {
        try {
            std::filesystem::create_directories(*upload_path);
        } catch (const std::exception &e) {
            throw std::runtime_error("Failed to create upload directory: " + std::string(e.what()));
        }
    }
    if (!std::filesystem::is_directory(*upload_path))
        throw std::runtime_error("Upload path is not a directory: " + *upload_path);
    if (allowed_methods.find("POST") == allowed_methods.end() && allow_upload)
        throw std::runtime_error("Upload is allowed but POST method is not enabled");
    if (allow_upload && !upload_path)
        throw std::runtime_error("Upload is enabled but upload_store directive is not set");
    
    std::string test_file = *upload_path + "/write_text.txt";
    std::ofstream test(test_file);
    if (!test)
        throw std::runtime_error("Failed to write to upload directory: " + *upload_path);
    test.close();
    std::filesystem::remove(test_file);
}

void LocationConfig::validateCGI() const {
    auto it = _CGIExtensions.find(*cgi_extension);
    CGIExtention ext = (it != _CGIExtensions.end()) ? it->second : CGIExtention::UNHANDLED;

    switch(ext) {
        case CGIExtention::PY:
            if (*cgi_path != "/usr/bin/python3")
                throw std::runtime_error("Unusual Python interpreter path: " + *cgi_path);
            break;
        default:
            throw std::runtime_error("Unhandled CGI extension: " + *cgi_extension);
    }
    if (!std::filesystem::exists(*cgi_path))
        throw std::runtime_error("CGI path does not exist: " + *cgi_path);
    if (access(cgi_path->c_str(), X_OK) != 0)
        throw std::runtime_error("CGI path is not executable: " + *cgi_path);
    if ((allowed_methods.find("POST") == allowed_methods.end()) && (allowed_methods.find("GET") == allowed_methods.end()))
        throw std::runtime_error("CGI is enabled but neither POST nor GET methods are allowed");
}