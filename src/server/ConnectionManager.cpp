#include "ConnectionManager.hpp"

ConnectionManager::ConnectionManager() : _running(false){}

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

bool ConnectionManager::hasActivity(struct pollfd *poll_fd, short event) const {
    return (poll_fd->revents & event);
}

bool ConnectionManager::isServerSocket(int fd) const {
    return (!_poll_fds.empty() && _poll_fds[0].fd == fd);
}

std::vector<struct pollfd> ConnectionManager::checkConnection() {
    std::vector<struct pollfd> active_fds;

    int ready = poll(_poll_fds.data(), _poll_fds.size(), _timeout);

    if (ready < 0) {
        if (errno == EINTR)
            return active_fds; // Interrupted by signal, returning empty vector
        else
            throw std::runtime_error("poll() failed");
    }

    if (ready > 0) {
        for (auto &poll_fd : _poll_fds) {
            if (poll_fd.revents != 0) {
                active_fds.push_back(poll_fd);
                std::cout << "Activity on fd: " << poll_fd.fd << std::endl;
            }
        }
    }
    return active_fds;
}