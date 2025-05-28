#include "Server.hpp"
#include "ClientHandler.hpp"
#include <arpa/inet.h>
#include <iostream>
#include <cerrno>
#include <cstring>

Server::Server(int port) : _server_fd(-1), _port(port), _host("0.0.0.0"), _running(false) {
    memset(&_address, 0, sizeof(_address));
    if (getrlimit(RLIMIT_NOFILE, &_rlim) == -1) {
        throw std::runtime_error("Failed to get file descriptor limit");
    }
    _clientHandler = new ClientHandler(*this, "./www", 60);
}

Server::~Server() {
    if (_server_fd >= 0) {
        close(_server_fd);
    }
    delete _clientHandler;
}

void Server::createSocket() {
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd == -1)
        throw std::runtime_error("Failed to create socket");
}

void Server::setSocketOptions() {
    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        if (close(_server_fd) < 0)
            std::cerr << "Failed to close socket" << std::endl;
        throw std::runtime_error("Failed to set socket options");
    }
}

void Server::bindSocket() {
    _address.sin_family = AF_INET;
    _address.sin_port = htons(_port);
    if (inet_pton(AF_INET, _host.c_str(), &_address.sin_addr) <= 0) {
        if (close(_server_fd) < 0)
            std::cerr << "Failed to close socket" << std::endl;
        throw std::runtime_error("Failed to convert host address");
    }
    if (bind(_server_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0) {
        if (close(_server_fd) < 0)
            std::cerr << "Failed to close socket" << std::endl;
        throw std::runtime_error("Failed to bind socket");
    }
}

void Server::startListening() {
    if (listen(_server_fd, SOMAXCONN) < 0) {
        if (close(_server_fd) < 0)
            std::cerr << "Failed to close socket" << std::endl;
        throw std::runtime_error("Failed to listen on socket");
    }
}

void Server::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        throw std::runtime_error("Failed to get socket flags");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error("Failed to set socket to non-blocking");
}

void Server::acceptNewConnection() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(_server_fd, (struct sockaddr *)&client_addr, &addr_len);
    
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            throw std::runtime_error("Failed to accept new connection: " + std::string(strerror(errno)));
        return;
    }
    
    if (_clientHandler->getClientCount() >= _rlim.rlim_cur - 10) {
        _clientHandler->addClient(client_fd, client_addr);
        _clientHandler->handleClientEvent(ClientHandler::ClientEventType::ERROR, client_fd);
        return;
    }
    
    try {
        setNonBlocking(client_fd);
        _clientHandler->addClient(client_fd, client_addr);
    } catch (const std::exception& e) {
        if (close(client_fd) < 0) {
            std::cerr << "Failed to close client socket: " << strerror(errno) << std::endl;
        }
        throw;
    }
}

void Server::start() {
    _running = true;
    createSocket();
    setSocketOptions();
    bindSocket();
    startListening();
    setNonBlocking(_server_fd);
    addConnection(_server_fd, POLLIN);
}

void Server::stop() {
    _running = false;
    const std::vector<int> clientFds = _clientHandler->getAllClientFds();
    for (size_t i = 0; i < clientFds.size(); i++) {
        try {
            _clientHandler->disconnectClient(clientFds[i]);
        } catch (const std::exception& e) {
            std::cerr << "Error disconnecting client: " << e.what() << std::endl;
        }
    }
    removeConnection(_server_fd);
    if (_server_fd >= 0) {
        if (close(_server_fd) < 0) {
            std::cerr << "Error closing server socket: " << strerror(errno) << std::endl;
        }
        _server_fd = -1;
    }
}

void Server::checkClientTimeouts() {
    if (_clientHandler) {
        _clientHandler->checkTimeouts();
    }
}

void Server::handleClientRead(int clientFd) {
    _clientHandler->handleClientEvent(ClientHandler::ClientEventType::READ, clientFd);
}

void Server::handleClientWrite(int clientFd) {
    _clientHandler->handleClientEvent(ClientHandler::ClientEventType::WRITE, clientFd);
}

bool Server::hasClient(int fd) const {
    return _clientHandler->hasClient(fd);
}

void Server::addConnection(int fd, short event) {
    struct pollfd poll_fd;
    poll_fd.fd = fd;
    poll_fd.events = event;
    poll_fd.revents = 0;
    _poll_fds.push_back(poll_fd);
}

void Server::removeConnection(int fd) {
    auto it = std::find_if(_poll_fds.begin(), _poll_fds.end(),
              [fd](const struct pollfd& pfd) {return pfd.fd == fd;});
    if (it != _poll_fds.end()) {
        _poll_fds.erase(it);
    }
}

std::vector<struct pollfd> Server::checkConnection() {
    std::vector<struct pollfd> active_fds;
    if (_poll_fds.empty())
        return active_fds;
    int ret = poll(_poll_fds.data(), _poll_fds.size(), _poll_timeout);
    if (ret < 0) {
        if (errno == EINTR)
            return active_fds;
        throw std::runtime_error("poll() failed: " + std::string(strerror(errno)));
    }
    if (ret > 0) {
        for (const struct pollfd& poll_fd : _poll_fds) {
            if (poll_fd.revents > 0) {
                active_fds.push_back(poll_fd);
            }
        }
    }
    return active_fds;
}

bool Server::hasActivity(const struct pollfd *poll_fd, short event) const {
    return (poll_fd->revents & event) == event;
}
