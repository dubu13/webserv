#include "Socket.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

Socket::Socket(int port, const std::string& host) 
    : _server_fd(-1), _port(port), _host(host) {
    memset(&_address, 0, sizeof(_address));
}

Socket::~Socket() {
    closeSocket();
}

void Socket::createSocket() {
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd == -1)
        throw SocketException("Failed to create socket");
}

void Socket::setSocketOptions() {
    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        if (close(_server_fd) < 0)
            std::cerr << "Failed to close socket" << std::endl;
        throw SocketException("Failed to set socket options");
    }
    std::cout << "Socket options set successfully." << std::endl;
}

void Socket::bindSocket() {
    // Configure the address structure
    _address.sin_family = AF_INET;
    _address.sin_port = htons(_port);
    
    // Convert string IP to network format
    if (inet_pton(AF_INET, _host.c_str(), &_address.sin_addr) <= 0) {
        if (close(_server_fd) < 0)
            std::cerr << "Failed to close socket" << std::endl;
        throw SocketException("Failed to convert host address");
    }
    
    if (bind(_server_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0) {
        if (close(_server_fd) < 0)
            std::cerr << "Failed to close socket" << std::endl;
        throw SocketException("Failed to bind socket");
    }
    std::cout << "Socket bound successfully to " << _host << ":" << _port << std::endl;
}

void Socket::startListening() {
    if (listen(_server_fd, SOMAXCONN) < 0) {
        if (close(_server_fd) < 0)
            std::cerr << "Failed to close socket" << std::endl;
        throw SocketException("Failed to listen on socket");
    }
    std::cout << "Socket is now listening." << std::endl;
}

int Socket::getFd() const {
    return _server_fd;
}

void Socket::closeSocket() {
    if (_server_fd != -1) {
        if (close(_server_fd) < 0) {
            std::cerr << "Failed to close socket: " << strerror(errno) << std::endl;
        } else {
            std::cout << "Socket closed successfully." << std::endl;
            _server_fd = -1; // Mark as closed
        }
    }
}

void Socket::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1) {
        if (close(fd) < 0)
            std::cerr << "Failed to close socket" << std::endl;
        throw SocketException("Failed to get socket flags");
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        if (close(fd) < 0)
            std::cerr << "Failed to close socket" << std::endl;
        throw SocketException("Failed to set socket to non-blocking");
    }
    std::cout << "Socket set to non-blocking mode." << std::endl;
}
