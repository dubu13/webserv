#include "server/ServerManager.hpp"
#include "utils/Logger.hpp"
#include <atomic>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

extern std::atomic<bool> g_running;

ServerManager::ServerManager() : _running(false) {}

ServerManager::~ServerManager() { stop(); }

void ServerManager::initializeServers(const Config &config) {
  const auto &serverConfigs = config.getServers();

  if (serverConfigs.empty())
    throw std::runtime_error("No server configurations found");

  for (const auto &[key, serverBlock] : serverConfigs) {
    try {
      auto server = std::make_unique<Server>(&serverBlock);
      _servers.push_back(std::move(server));

      for (const auto &listen : serverBlock.listenDirectives) {
        std::string host = listen.first.empty() ? "*" : listen.first;
        int port = listen.second;

        std::stringstream ss;
        ss << host << ":" << port;
        std::string hostPort = ss.str();

        if (_hostPortMap.find(hostPort) == _hostPortMap.end())
          _hostPortMap[hostPort] = _servers.size() - 1;
      }
    } catch (const std::exception &e) {
      Logger::logf<LogLevel::ERROR>("Failed to initialize server for " + key +
                                    ": " + e.what());
      throw;
    }
  }

  Logger::logf<LogLevel::INFO>("Initialized %s servers", std::to_string(_servers.size()).c_str());
}

void ServerManager::setupServerSockets() {
  for (size_t i = 0; i < _servers.size(); ++i) {
    int serverFd = _servers[i]->setupSocket();
    if (serverFd < 0) {
      throw std::runtime_error("Failed to setup socket for server " +
                               std::to_string(i));
    }

    _socketToServerMap[serverFd] = i;
    _poller.add(serverFd, POLLIN);

    Logger::logf<LogLevel::INFO>("Server %s listening on fd %s",
                                 std::to_string(i).c_str(),
                                 std::to_string(serverFd).c_str());
  }
  Logger::logf<LogLevel::INFO>("All server sockets initialized");
}
bool ServerManager::start() {
  if (_running)
    return true;

  _running = false;

  try {
    setupServerSockets();

    _running = true;
    Logger::logf<LogLevel::INFO>("Starting all servers...");

    while (_running && g_running.load()) {
      processEvents(1000);
      checkAllTimeouts();
    }

    Logger::logf<LogLevel::INFO>("Server manager stopped");
    return true;
  } catch (const std::exception &e) {
    Logger::logf<LogLevel::ERROR>("Error in server manager: %s", e.what());
    return false;
  }
}

void ServerManager::stop() {
  _running = false;

  for (auto &server : _servers) {
    server->stop();
  }

  _socketToServerMap.clear();
}

bool ServerManager::processEvents(int timeout) {
  try {
    auto activeFds = _poller.poll(timeout);

    for (const auto &pfd : activeFds) {
      try {
        dispatchEvent(pfd);
      } catch (const std::exception &e) {
        Logger::logf<LogLevel::ERROR>("Error dispatching event: %s", e.what()); }
    }

    return true;
  } catch (const std::exception &e) {
    if (errno == EINTR) {
      Logger::logf<LogLevel::WARN>("Poll interrupted by signal");
      return true;
    }
    Logger::logf<LogLevel::ERROR>("Error processing events: %s", e.what());
    return false;
  }
}

void ServerManager::dispatchEvent(const struct pollfd &pfd) {
  auto serverIt = _socketToServerMap.find(pfd.fd);

  if (serverIt != _socketToServerMap.end()) {
    size_t serverIndex = serverIt->second;
    int clientFd = _servers[serverIndex]->acceptConnection();
    if (clientFd > 0)
      _poller.add(clientFd, POLLIN);
    return;
  }

  for (auto &server : _servers) {
    if (server->hasClient(pfd.fd)) {
      server->handleClient(pfd.fd);
      if (!server->hasClient(pfd.fd))
        _poller.remove(pfd.fd);
      return;
    }
  }

  Logger::logf<LogLevel::WARN>("Unhandled socket event for fd: %d", pfd.fd);
  _poller.remove(pfd.fd);
  close(pfd.fd);
}

void ServerManager::checkAllTimeouts() {
  for (auto &server : _servers) {
    server->checkTimeouts();
  }
}
