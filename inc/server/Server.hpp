#pragma once
#include "Poller.hpp"
#include "config/Config.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string>
#include <sys/resource.h>
#include <sys/socket.h>
#include <unistd.h>
#include <memory>
class ClientHandler;
class Server {
private:
  int _server_fd;
  struct sockaddr_in _address;
  ServerBlock _config;
  Poller _poller;
  std::unique_ptr<ClientHandler> _clientHandler;
  rlimit _rlim;
  bool _running;
  void setupSocket();
  void setNonBlocking(int fd);
public:
  Server(const ServerBlock &config);
  ~Server();
  void run();
  void stop();
  int getServerFd() const { return _server_fd; }
  bool isServerSocket(int fd) const { return fd == _server_fd; }
  const ServerBlock &getConfig() const { return _config; }
  void acceptConnection();
  void handleClientEvent(int fd, short events);
  Poller &getPoller() { return _poller; }
};
