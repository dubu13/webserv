#include "config/ServerBlock.hpp"
#include "utils/Utils.hpp"

const LocationBlock *ServerBlock::getLocation(const std::string &path) const {
  return findBestLocationMatch(path);
}

const LocationBlock *
ServerBlock::findBestLocationMatch(const std::string &path) const {
  std::string cleanPath = HttpUtils::sanitizePath(path);

  if (cleanPath == "/") {
    auto it = locations.find("/");
    if (it != locations.end())
      return &it->second;
    return nullptr;
  }

  const LocationBlock *bestMatch = nullptr;
  size_t bestMatchLength = 0;

  for (const auto &[prefix, location] : locations) {
    if (prefix == "/" && cleanPath != "/")
      continue;
    if (cleanPath.find(prefix) == 0) {
      if (prefix.length() > 1 && cleanPath.length() > prefix.length() &&
          cleanPath[prefix.length()] != '/')
        continue;
      if (prefix.length() > bestMatchLength) {
        bestMatch = &location;
        bestMatchLength = prefix.length();
      }
    }
  }
  return bestMatch;
}
