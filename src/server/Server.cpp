#include "Server.hpp"
#include "ClientHandler.hpp"
#include <iostream>
#include <cerrno>
#include <stdexcept>
extern bool g_running;
Server::Server(const ServerConfig& config)
    : _server_fd(-1), _config(config), _running(false) {
    memset(&_address, 0, sizeof(_address));
    if (getrlimit(RLIMIT_NOFILE, &_rlim) == -1) {
        throw std::runtime_error("Failed to get file descriptor limit");
    }
    _clientHandler = new ClientHandler(*this, _config.root, 60);
}
Server::~Server() {
    if (_server_fd >= 0) {
        close(_server_fd);
    }
    delete _clientHandler;
}
void Server::setupSocket() {
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd == -1) {
        throw std::runtime_error("Failed to create socket");
    }
    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(_server_fd);
        throw std::runtime_error("Failed to set socket options");
    }
    _address.sin_family = AF_INET;
    _address.sin_port = htons(_config.port);
    if (inet_pton(AF_INET, _config.host.c_str(), &_address.sin_addr) <= 0) {
        close(_server_fd);
        throw std::runtime_error("Failed to convert host address");
    }
    if (bind(_server_fd, (struct sockaddr*)&_address, sizeof(_address)) < 0) {
        close(_server_fd);
        throw std::runtime_error("Failed to bind socket");
    }
    if (listen(_server_fd, SOMAXCONN) < 0) {
        close(_server_fd);
        throw std::runtime_error("Failed to listen on socket");
    }
    setNonBlocking(_server_fd);
    std::cout << "Server listening on " << _config.host << ":" << _config.port << std::endl;
}
void Server::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("Failed to set non-blocking mode");
    }
}
void Server::run() {
    _running = true;
    setupSocket();
    _poller.addFd(_server_fd, POLLIN);
    std::cout << "Server listening on " << _config.host << ":" << _config.port << std::endl;
    while (g_running && _running) {
        try {
            auto active_fds = _poller.poll();
            for (const auto& pfd : active_fds) {
                if (isServerSocket(pfd.fd)) {
                    if (_poller.hasActivity(pfd, POLLIN)) {
                        acceptConnection();
                    }
                } else {
                    handleClientEvent(pfd.fd, pfd.revents);
                }
            }
            _clientHandler->checkTimeouts();
        } catch (const std::exception& e) {
            std::cerr << "Server error: " << e.what() << std::endl;
        }
    }
}
void Server::acceptConnection() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(_server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Accept failed: " << strerror(errno) << std::endl;
        }
        return;
    }
    if (_clientHandler->getClientCount() >= _rlim.rlim_cur - 10) {
        close(client_fd);
        return;
    }
    try {
        setNonBlocking(client_fd);
        _clientHandler->addClient(client_fd, client_addr);
    } catch (const std::exception& e) {
        close(client_fd);
        std::cerr << "Failed to add client: " << e.what() << std::endl;
    }
}
void Server::handleClientEvent(int fd, short events) {
    if (events & (POLLERR | POLLHUP)) {
        _clientHandler->removeClient(fd);
    } else if (events & POLLIN) {
        _clientHandler->handleRead(fd);
    } else if (events & POLLOUT) {
        _clientHandler->handleWrite(fd);
    }
}
void Server::stop() {
    _running = false;
    if (_server_fd >= 0) {
        close(_server_fd);
        _server_fd = -1;
    }
}
