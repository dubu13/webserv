#pragma once

#include "HTTP/handlers/MethodDispatcher.hpp"
#include "HTTP/routing/RequestRouter.hpp"
#include "Poller.hpp"
#include "ServerBlock.hpp"
#include "resource/CGIHandler.hpp"
#include <ctime>
#include <map>
#include <string>
#include <vector>

class Server {
private:
  int _serverFd;
  bool _running;
  Poller _poller;
  std::map<int, time_t> _clients;
  std::map<int, std::string> _clientBuffers;
  std::map<int, int> _cgiToClient;
  const ServerBlock *_config;
  RequestRouter _router;

public:
  explicit Server(const ServerBlock *config);
  ~Server();

  int setupSocket();
  void stop() { _running = false; }

  int acceptConnection();
  void handleClient(int fd);
  void checkTimeouts();

  bool hasClient(int fd) const { return _clients.find(fd) != _clients.end(); }
  void closeClient(int fd) { removeClient(fd); }

private:
  void removeClient(int fd);
  void sendErrorToClient(int fd, int statusCode);
  void sendResponseToClient(int clientFd, const std::string& response);
};
