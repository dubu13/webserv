#include "Server.hpp"
#include <arpa/inet.h> // for inet_ntop

Server::Server() : _socket() {
    if (getrlimit(RLIMIT_NOFILE, &_rlim) == -1) {
        throw std::runtime_error("Failed to get file descriptor limit");
    }
    std::cout << "System allows " << _rlim.rlim_cur << " file descriptors." << std::endl;
    std::cout << "Server will listen on port " << port << std::endl;
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

// size_t Server::getClientCount() const {
//     return _clientFds.size();
// }

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

void Server::handleClientRead(int client_fd){
    char buffer[4096] = {0}; // Initialize buffer to zeros
    
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1); // Leave space for null terminator

    if (bytes_read < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            disconnectClient(client_fd);
            throw std::runtime_error("Failed to read from client socket");
        }
        return; // Return without processing if there was an error
    }
    
    if (bytes_read == 0) {
        std::cout << "Client " << client_fd << " disconnected." << std::endl;
        disconnectClient(client_fd);
        return;
    }

    // Ensure null termination
    buffer[bytes_read] = '\0';
    
    // Append to the incoming data buffer
    _incomingData[client_fd].append(buffer, bytes_read);
    
    // Check for complete HTTP request (ends with \r\n\r\n)
    if (_incomingData[client_fd].find("\r\n\r\n") != std::string::npos) {
        std::cout << "Complete HTTP request received from client " << client_fd << std::endl;
        
        try {
            // Create a simple HTTP response (for now)
            std::string httpResponse = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: 22\r\n"
                "\r\n"
                "<h1>Hello, World!</h1>";
            
            // Store the response for sending
            _outgoingData[client_fd] = httpResponse;
            
            // Switch the client connection from read to write mode
            _connectionManager.removeConnection(client_fd);
            _connectionManager.addConnection(client_fd, POLLOUT);
            
            // Clear the incoming data buffer for this client
            _incomingData[client_fd].clear();
        }
        catch (const std::exception& e) {
            std::cerr << "Error processing request: " << e.what() << std::endl;
            
            // Send an error response
            std::string errorResponse = 
                "HTTP/1.1 500 Internal Server Error\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: 36\r\n"
                "\r\n"
                "<h1>500 Internal Server Error</h1>";
            
            _outgoingData[client_fd] = errorResponse;
            _connectionManager.removeConnection(client_fd);
            _connectionManager.addConnection(client_fd, POLLOUT);
            _incomingData[client_fd].clear();
        }
    }
    // If we don't have a complete request yet, keep reading
}

void Server::handleClientWrite(int client_fd) {
    if (_outgoingData.find(client_fd) == _outgoingData.end() || _outgoingData[client_fd].empty())
        return;

    std::string& data = _outgoingData[client_fd];

    ssize_t bytes_written = write(client_fd, data.c_str(), data.size());

    if (bytes_written < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            disconnectClient(client_fd);
            throw std::runtime_error("Failed to write to client socket");
        }
        return;
    }
    if (bytes_written == 0) {
        std::cout << "Client " << client_fd << " disconnected." << std::endl;
        disconnectClient(client_fd);
        return;
    }

    data.erase(0, bytes_written);
    _connectionManager.removeConnection(client_fd); 
    if (!data.empty())
        _connectionManager.addConnection(client_fd, POLLOUT);
    else {
        _outgoingData.erase(client_fd);
        _connectionManager.addConnection(client_fd, POLLIN);
        std::cout << "Data sent to client " << client_fd << std::endl;
    }
}
