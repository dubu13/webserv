#pragma once
#include "HTTPHandler.hpp"
#include "Client.hpp"
#include <map>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <ctime>
#include <vector>
class Server;
class ClientHandler {
public:
  enum class ClientEventType { READ, WRITE, ERROR, TIMEOUT };
private:
  Server &_server;
  std::map<int, Client> _clients;
  HTTPHandler _httpHandler;
  time_t _clientTimeout;
  void handleRead(int clientFd);
  void handleWrite(int clientFd);
  void handleError(int clientFd);
  void handleTimeout(int clientFd);
  void sendErrorResponse(int clientFd, HTTP::StatusCode status);
  bool processReadResult(int clientFd, Client& client, ssize_t bytes_read);
  void processCompleteRequest(int clientFd, Client& client);
  bool processWriteResult(int clientFd, Client& client, ssize_t bytes_written);
  void handleWriteCompletion(int clientFd, Client& client);
public:
  ClientHandler(Server &server,
                const std::string &webRoot = "./www", time_t clientTimeout = 30);
  ~ClientHandler();
  void addClient(int clientFd, const struct sockaddr_in &clientAddr);
  void disconnectClient(int clientFd);
  bool hasClient(int clientFd) const;
  void checkTimeouts();
  size_t getClientCount() const;
  std::vector<int> getAllClientFds() const;
  void handleClientEvent(ClientEventType eventType, int clientFd);
};
