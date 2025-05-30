#pragma once
#include "HTTPHandler.hpp"
#include <arpa/inet.h>
#include <ctime>
#include <map>
#include <netinet/in.h>
#include <string>
#include <vector>

class Server;

struct ClientData {
  int socketFd;
  struct sockaddr_in address;
  std::string incomingData;
  std::string outgoingData;
  bool keepAlive;
  time_t lastActivity;
  
  ClientData(int fd, const struct sockaddr_in &addr)
    : socketFd(fd), address(addr), keepAlive(false), lastActivity(time(NULL)) {}
    
  std::string getIpAddress() const {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &address.sin_addr, ip, INET_ADDRSTRLEN);
    return std::string(ip);
  }
  
  int getPort() const { return ntohs(address.sin_port); }
  void updateActivity() { lastActivity = time(NULL); }
  bool hasTimedOut(time_t timeout) const { return (time(NULL) - lastActivity) > timeout; }
  bool hasDataToWrite() const { return !outgoingData.empty(); }
};

class ClientHandler {
private:
  Server &_server;
  std::map<int, ClientData> _clients;
  HTTPHandler _httpHandler;
  time_t _timeout;
  
  void processRequest(int fd, ClientData &client);
  void sendError(int fd, int status);
  bool hasCompleteRequest(const std::string &data) const;
  ssize_t readClientData(int fd, std::string &incomingData, size_t maxSize);
  ssize_t writeClientData(int fd, std::string &outgoingData);
  
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
