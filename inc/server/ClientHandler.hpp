#pragma once

#include "ConnectionManager.hpp"
#include "HTTPHandler.hpp"
#include "ResourceHandler.hpp"
#include <functional>
#include <map>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

class ClientHandler {
public:
  enum class ClientEventType { READ, WRITE, ERROR, TIMEOUT };

private:
  typedef void (ClientHandler::*EventHandlerFunction)(int);

  ConnectionManager &_connectionManager;
  std::unordered_set<int> _clientFds;
  std::unordered_map<int, std::string> _incomingData;
  std::unordered_map<int, std::string> _outgoingData;
  HTTPHandler _httpHandler;
  ResourceHandler *_resourceHandler;

  std::map<ClientEventType, EventHandlerFunction> _eventHandlers;

  void handleRead(int clientFd);
  void handleWrite(int clientFd);
  void handleError(int clientFd);
  void handleTimeout(int clientFd);

  void sendErrorResponse(int clientFd, HTTP::StatusCode status);

public:
  ClientHandler(ConnectionManager &connectionManager,
                const std::string &webRoot = "./www");
  ~ClientHandler();

  void addClient(int clientFd, const struct sockaddr_in &clientAddr);
  void disconnectClient(int clientFd);
  bool hasClient(int clientFd) const;
  const std::unordered_set<int> &getClients() const;

  void handleClientEvent(ClientEventType eventType, int clientFd);

  void handleClientRead(int clientFd) {
    handleClientEvent(ClientEventType::READ, clientFd);
  }
  void handleClientWrite(int clientFd) {
    handleClientEvent(ClientEventType::WRITE, clientFd);
  }
};
