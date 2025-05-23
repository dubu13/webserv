#pragma once

#include <string>
#include <unordered_map>

class FileType {
private:
  static std::unordered_map<std::string, std::string> _mimeTypes;
  static std::unordered_map<std::string, std::string> _cgiHandlers;

public:
  static std::string getMimeType(const std::string &path);
  static bool isCGI(const std::string &path);
  static std::string getCGIHandler(const std::string &path);
  static std::string getExtension(const std::string &path);
  static void registerMimeType(const std::string &extension,
                               const std::string &mimeType);
  static void registerCGIHandler(const std::string &extension,
                                 const std::string &handler);
};
