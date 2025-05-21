#pragma once

#include "Socket.hpp"
#include "ConnectionManager.hpp"
#include <sys/resource.h> // for struct rlimit
#include <unordered_set>
#include <fcntl.h> // for fcntl
#include <unistd.h> // for close
#include <unordered_map>

class Server {
    private:
        Socket _socket;
        ConnectionManager _connectionManager;
        std::unordered_set<int> _clientFds;
        std::unordered_map<int, std::string> _incomingData; // incoming data from clients
        std::unordered_map<int, std::string> _outgoingData; // outgoing data from clients
        rlimit _rlim;
        bool _running;

    public:
        Server();
        ~Server();

        void acceptNewConnection();
        void disconnectClient(int client_fd);
        bool hasClient(int client_fd) const;
        // size_t getClientCount() const;
        const std::unordered_set<int>& getClients() const;
        void start();
        void stop();

        // client data handling
        void handleClientRead(int client_fd);
        void handleClientWrite(int client_fd);

        int getSocketFd() const { return _socket.getFd(); }
        ConnectionManager& getConnectionManager() { return _connectionManager; }
        bool isServerSocket(int fd) const { return fd == _socket.getFd(); }
};
