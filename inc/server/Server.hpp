#pragma once
#include <sys/resource.h>
#include <unordered_set>
#include <fcntl.h>
#include <unistd.h>
#include <unordered_map>
#include <netinet/in.h>
#include <poll.h>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>

class ClientHandler;

class Server {
private:
    int _server_fd;
    struct sockaddr_in _address;
    int _port;
    std::string _host;
    std::vector<struct pollfd> _poll_fds;
    static const int _poll_timeout = 1000;
    ClientHandler* _clientHandler;
    rlimit _rlim;
    bool _running;

    // Socket management methods
    void createSocket();
    void setSocketOptions();
    void bindSocket();
    void startListening();
public:
    Server(int port = 8080);
    ~Server();
    void start();
    void stop();
    void acceptNewConnection();
    void checkClientTimeouts();
    int getSocketFd() const { return _server_fd; }
    bool isServerSocket(int fd) const { return fd == _server_fd; }
    void handleClientRead(int clientFd);
    void handleClientWrite(int clientFd);
    bool hasClient(int fd) const;
    void addConnection(int fd, short event);
    void removeConnection(int fd);
    std::vector<struct pollfd> checkConnection();
    bool hasActivity(const struct pollfd *poll_fd, short event) const;

    static void setNonBlocking(int fd);
};
