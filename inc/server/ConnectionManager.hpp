#pragma once

#include <poll.h>
#include <vector>
#include <algorithm>

class ConnectionManager {
    private:
        std::vector<struct pollfd> _poll_fds;
        static const int _timeout = 1000; // 1 second
        bool _running;

    public:
        ConnectionManager();
        ~ConnectionManager();
    
        void addConnection(int fd, short event);
        void removeConnection(int fd);
        bool hasActivity(struct pollfd *poll_fd, short event) const;
        bool isServerSocket(int fd) const;
        std::vector<struct pollfd> checkConnection();
};
#include "Server.hpp"
