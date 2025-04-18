#include "Socket.hpp"

Socket::Socket(int port) :_server_fd(-1), _port(port){
    memset(&_address, 0, sizeof(_address));
}

Socket::~Socket() {
    if (_server_fd != -1) {
        if (close(_server_fd) < 0)
            throw std::runtime_error("Failed to close socket");
    }
}

void Socket::createSocket() {
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd == -1)
        throw std::runtime_error("Failed to create socket");
}

void Socket::setSocketOptions() {
    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        if (close(_server_fd) < 0)
            throw std::runtime_error("Failed to close socket");
        throw std::runtime_error("Failed to set socket options");
    }
    std::cout << "Socket options set successfully." << std::endl;
}

void Socket::setupAddress() {
    if (bind(_server_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0) {
        if (close(_server_fd) < 0)
            throw std::runtime_error("Failed to close socket");
        throw std::runtime_error("Failed to bind socket");
    }
    std::cout << "Socket bound successfully." << std::endl;
}

void Socket::bindSocket() {
    if (bind(_server_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0) {
        if (close(_server_fd) < 0)
            throw std::runtime_error("Failed to close socket");
        throw std::runtime_error("Failed to bind socket");
    }
    std::cout << "Socket bound successfully" << std::endl;
}

void Socket::startListening() {
    if (listen(_server_fd, SOMAXCONN) < 0) {
        if (close(_server_fd) < 0)
            throw std::runtime_error("Failed to close socket");
        throw std::runtime_error("Failed to listen on socket");
    }
    std::cout << "Socket is now listening." << std::endl;
}

int Socket::getFd() const {
    return _server_fd;
}

void Socket::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1) {
        if (close(fd) < 0)
            throw std::runtime_error("Failed to close socket");
        throw std::runtime_error("Failed to get socket flags");
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        if (close(fd) < 0)
            throw std::runtime_error("Failed to close socket");
        throw std::runtime_error("Failed to set socket to non-blocking");
    }
    std::cout << "Socket set to non-blocking mode." << std::endl;
}