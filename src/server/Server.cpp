#include "Server.hpp"
#include "ClientHandler.hpp" // Include here instead of in the header
#include <arpa/inet.h>
#include <iostream>

Server::Server(const ServerConfig& config) 
    : _socket(config.port), _running(false), _config(config) {
    if (getrlimit(RLIMIT_NOFILE, &_rlim) == -1) {
        throw std::runtime_error("Failed to get file descriptor limit");
    }
    std::cout << "System allows " << _rlim.rlim_cur << " file descriptors." << std::endl;
    std::cout << "Server will listen on " << config.host << ":" << config.port << std::endl;
    
    // Create the ClientHandler with configuration
    _clientHandler = new ClientHandler(_connectionManager, config.root, 60);  // TODO: make timeout configurable
}

Server::~Server() {
    // Clean up the ClientHandler
    delete _clientHandler;
}

void Server::acceptNewConnection() {
    if (_clientHandler->getClients().size() >= _rlim.rlim_cur - 10) { //10 is for safety buffer
        std::cerr << "Max clients reached" << std::endl;
        // Accept but immediately close connection with error message
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(_socket.getFd(), (struct sockaddr *)&client_addr, &addr_len);
        
        if (client_fd >= 0) {
            // Send a service unavailable error response
            _clientHandler->addClient(client_fd, client_addr);
            _clientHandler->handleClientEvent(ClientHandler::ClientEventType::ERROR, client_fd);
        }
        return;
    }
    
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(_socket.getFd(), (struct sockaddr *)&client_addr, &addr_len);

    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            throw SocketException("Failed to accept new connection: " + std::string(strerror(errno)));
        return;
    }

    try {
        Socket::setNonBlocking(client_fd);
        
        // Add client using ClientHandler
        _clientHandler->addClient(client_fd, client_addr);
    } catch (const std::exception& e) {
        // Close the socket if anything went wrong during setup
        if (close(client_fd) < 0) {
            std::cerr << "Failed to close client socket: " << strerror(errno) << std::endl;
        }
        throw; // Re-throw the exception
    }
}

void Server::start() {
    std::cout << "Server started, waiting for connections..." << std::endl;
    
    _running = true;

    _socket.createSocket();
    _socket.setSocketOptions();
    _socket.bindSocket();
    _socket.startListening();
    Socket::setNonBlocking(_socket.getFd());

    _connectionManager.addConnection(_socket.getFd(), POLLIN);
}

void Server::stop() {
    _running = false;
    
    // First disconnect all clients
    // Get a copy of client file descriptors to avoid modification during iteration
    const std::unordered_set<int> clients = _clientHandler->getClients();
    std::vector<int> clientFds(clients.begin(), clients.end());
    
    for (size_t i = 0; i < clientFds.size(); i++) {
        try {
            _clientHandler->disconnectClient(clientFds[i]);
        } catch (const std::exception& e) {
            std::cerr << "Error disconnecting client: " << e.what() << std::endl;
        }
    }
    
    // Remove the server socket from the connection manager
    _connectionManager.removeConnection(_socket.getFd());
    
    // Close the server socket
    try {
        _socket.closeSocket();
    } catch (const std::exception& e) {
        std::cerr << "Error closing server socket: " << e.what() << std::endl;
    }
    
    std::cout << "Server stopped." << std::endl;
}

// Check for client timeouts
void Server::checkClientTimeouts() {
    if (_clientHandler) {
        _clientHandler->checkTimeouts();
    }
}

// Implement the methods that delegate to ClientHandler
void Server::handleClientRead(int clientFd) {
    _clientHandler->handleClientRead(clientFd);
}

void Server::handleClientWrite(int clientFd) {
    _clientHandler->handleClientWrite(clientFd);
}

// Implement the hasClient method
bool Server::hasClient(int fd) const {
    return _clientHandler->hasClient(fd);
}
