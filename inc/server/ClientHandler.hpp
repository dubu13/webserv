#pragma once
#include "HTTPHandler.hpp"
#include "server/Client.hpp"
#include <ctime>
#include <map>
#include <string>
#include <vector>

class Server;

class ClientHandler {
private:
  Server &_server;
  std::map<int, server::Client> _clients;
  HTTPHandler _httpHandler;
  time_t _timeout;
  
  void processRequest(int fd, server::Client &client);
  void sendError(int fd, int status);
  bool hasCompleteRequest(const std::string &data) const;
  ssize_t readClientData(int fd, server::Client &client, size_t maxSize);
  ssize_t writeClientData(int fd, server::Client &client);
  
public:
  ClientHandler(Server &server, const std::string &webRoot, time_t timeout = 60);
  ~ClientHandler();
  void addClient(int fd, const struct sockaddr_in &addr);
  void removeClient(int fd);
  bool hasClient(int fd) const;
  size_t getClientCount() const;
  void handleRead(int fd);
  void handleWrite(int fd);
  void checkTimeouts();
};
