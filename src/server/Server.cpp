#include "Server.hpp"

Server::Server(int port) : _socket(port), _running(false) {
    if (getrlimit(RLIMIT_NOFILE, &_rlim) == -1) {
        throw std::runtime_error("Failed to get file descriptor limit");
    }
    std::cout << "System allows " << _rlim.rlim_cur << " file descriptors." << std::endl;
}

Server::~Server() {
    for (int fd : _clientFds)
        if (close(fd) < 0)
            std::cerr << "Failed to close client socket" << std::endl;
    _clientFds.clear();
}

void Server::acceptNewConnection() {
    if (_clientFds.size() >= _rlim.rlim_cur - 10) //10 is for safety buffer
        throw std::runtime_error("Max clients reached");
    
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(_socket.getFd(), (struct sockaddr *)&client_addr, &addr_len);

    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            throw std::runtime_error("Failed to accept new connection");
        return;
    }

    Socket::setNonBlocking(client_fd);
    _clientFds.insert(client_fd);

    char client_ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip)) == NULL)
        throw std::runtime_error("Failed to convert client IP address");
    std::cout << "New connection from " << client_ip << ":" << ntohs(client_addr.sin_port) << "fd: " << client_fd << std::endl;
}

void Server::disconnectClient(int client_fd) {
    if (_clientFds.erase(client_fd) == 0)
        throw std::runtime_error("Attempted to disconnect non-existent client");
    if (close(client_fd) < 0)
        throw std::runtime_error("Failed to close client socket");

    std::cout << "Client disconnected, fd: " << client_fd << std::endl;
}

bool Server::hasClient(int client_fd) const {
    return _clientFds.find(client_fd) != _clientFds.end();
}

size_t Server::getClientCount() const {
    return _clientFds.size();
}

const std::unordered_set<int>& Server::getClients() const {
    return _clientFds;
}

void Server::start() {
    _running = true;

    _socket.createSocket();
    _socket.setSocketOptions();
    _socket.bindSocket();
    _socket.startListening();
    Socket::setNonBlocking(_socket.getFd());
    /*add server socket to poll_fds
        get active sockets
        for each active socket:
            if it's the server socket, accept new connection
            else handle client activity
    */ 
    

    std::cout << "Server started, waiting for connections..." << std::endl;
}

void Server::stop() {
    _running = false;
    for (int fd : _clientFds) {
        if (close(fd) < 0)
            throw std::runtime_error("Failed to close client socket");
    }
    _clientFds.clear();
    std::cout << "Server stopped." << std::endl;
}