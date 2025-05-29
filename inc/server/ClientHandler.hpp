#pragma once
#include "Client.hpp"
#include "HTTPHandler.hpp"
#include <map>
#include <vector>
#include <netinet/in.h>
#include <ctime>
class Server;
class ClientHandler {
private:
    Server& _server;
    std::map<int, Client> _clients;
    HTTPHandler _httpHandler;
    time_t _timeout;
    void processRequest(int fd, Client& client);
    void sendResponse(int fd, Client& client);
    void sendError(int fd, int status);
public:
    ClientHandler(Server& server, const std::string& webRoot, time_t timeout = 60);
    ~ClientHandler();
    void addClient(int fd, const struct sockaddr_in& addr);
    void removeClient(int fd);
    bool hasClient(int fd) const;
    size_t getClientCount() const;
    void handleRead(int fd);
    void handleWrite(int fd);
    void checkTimeouts();
};
