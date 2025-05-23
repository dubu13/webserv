#include "ConnectionManager.hpp"
#include "Socket.hpp" // For SocketException
#include <iostream>
#include <cerrno>
#include <cstring> // For strerror
#include <stdexcept>

ConnectionManager::ConnectionManager() {}

ConnectionManager::~ConnectionManager() {
    _poll_fds.clear();
}

void ConnectionManager::addConnection(int fd, short event) {
    struct pollfd poll_fd;
    poll_fd.fd = fd;
    poll_fd.events = event;
    poll_fd.revents = 0;

    _poll_fds.push_back(poll_fd);
    std::cout << "Added fd: " << fd << " to monitoring with event: " << event << std::endl;
}

void ConnectionManager::removeConnection(int fd) {
    auto it = std::find_if(_poll_fds.begin(), _poll_fds.end(),
              [fd](const struct pollfd& pfd) {return pfd.fd == fd;});

    if (it != _poll_fds.end()) {
        _poll_fds.erase(it);
        std::cout << "Removed fd: " << fd << " from monitoring" << std::endl;
    }
}

std::vector<struct pollfd> ConnectionManager::checkConnection() {
    std::vector<struct pollfd> active_fds;

    if (_poll_fds.empty())
        return active_fds;

    int ret = poll(_poll_fds.data(), _poll_fds.size(), _timeout);

    if (ret < 0) {
        if (errno == EINTR)
            return active_fds;
        throw SocketException("poll() failed: " + std::string(strerror(errno)));
    }

    if (ret > 0) {
        for (const struct pollfd& poll_fd : _poll_fds) {
            if (poll_fd.revents > 0) {
                std::cout << "Activity on fd: " << poll_fd.fd << std::endl;
                active_fds.push_back(poll_fd);
            }
        }
    }

    return active_fds;
}

bool ConnectionManager::hasActivity(const struct pollfd *poll_fd, short event) const {
    return (poll_fd->revents & event) == event;
}

bool ConnectionManager::isServerSocket(int fd) const {
    return (!_poll_fds.empty() && _poll_fds[0].fd == fd);
}
