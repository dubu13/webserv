#pragma once

#include "Socket.hpp"
#include <sys/resource.h> // for struct rlimit
#include <unordered_set> // for std::unordered_set
#include <arpa/inet.h> // for inet_ntop
#include <fcntl.h> // for fcntl
#include <unistd.h> // for close

class Server {
    private:
        Socket _socket;
        std::unordered_set<int> _clientFds;
        rlimit _rlim;
        bool _running;

    public:
        Server(int port);
        ~Server();

        void acceptNewConnection();
        void disconnectClient(int client_fd);
        bool hasClient(int client_fd) const;
        size_t getClientCount() const;
        const std::unordered_set<int>& getClients() const;
        void start();
        void stop();
};