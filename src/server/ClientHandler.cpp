#include "ClientHandler.hpp"
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <cstring> // Add this for strerror function

ClientHandler::ClientHandler(ConnectionManager& connectionManager, const std::string& webRoot)
    : _connectionManager(connectionManager), _httpHandler(webRoot) {
    // Initialize the event handler map
    _eventHandlers[ClientEventType::READ] = &ClientHandler::handleRead;
    _eventHandlers[ClientEventType::WRITE] = &ClientHandler::handleWrite;
    _eventHandlers[ClientEventType::ERROR] = &ClientHandler::handleError;
    _eventHandlers[ClientEventType::TIMEOUT] = &ClientHandler::handleTimeout;
    
    // Get reference to the ResourceHandler from HTTPHandler
    _resourceHandler = &_httpHandler.getResourceHandler();
}

ClientHandler::~ClientHandler() {
    for (int fd : _clientFds) {
        if (close(fd) < 0)
            std::cerr << "Failed to close client socket" << std::endl;
    }
    _clientFds.clear();
}

void ClientHandler::addClient(int clientFd, const struct sockaddr_in& clientAddr) {
    _clientFds.insert(clientFd);
    
    // Add new client to poll monitoring
    _connectionManager.addConnection(clientFd, POLLIN);
    
    char client_ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &clientAddr.sin_addr, client_ip, sizeof(client_ip)) == NULL)
        throw std::runtime_error("Failed to convert client IP address");
    std::cout << "New connection from " << client_ip << ":" << ntohs(clientAddr.sin_port) << " fd: " << clientFd << std::endl;
}

void ClientHandler::disconnectClient(int clientFd) {
    // Remove from connection manager first
    _connectionManager.removeConnection(clientFd);
    
    // Clean up data structures
    _incomingData.erase(clientFd);
    _outgoingData.erase(clientFd);
    
    // Remove from client set and close
    if (_clientFds.erase(clientFd) == 0)
        throw std::runtime_error("Attempted to disconnect non-existent client");
    if (close(clientFd) < 0)
        throw std::runtime_error("Failed to close client socket");

    std::cout << "Client disconnected, fd: " << clientFd << std::endl;
}

bool ClientHandler::hasClient(int clientFd) const {
    return _clientFds.find(clientFd) != _clientFds.end();
}

const std::unordered_set<int>& ClientHandler::getClients() const {
    return _clientFds;
}

// Main event handler function that uses the function pointer map
void ClientHandler::handleClientEvent(ClientEventType eventType, int clientFd) {
    auto handlerIt = _eventHandlers.find(eventType);
    if (handlerIt != _eventHandlers.end()) {
        // Call the correct handler function through the function pointer
        (this->*(handlerIt->second))(clientFd);
    } else {
        std::cerr << "No handler registered for event type on client " << clientFd << std::endl;
    }
}

// Individual event handlers
void ClientHandler::handleRead(int clientFd) {
    char buffer[4096] = {0}; // Initialize buffer to zeros
    
    ssize_t bytes_read = read(clientFd, buffer, sizeof(buffer) - 1);
    std::cout << "Read " << bytes_read << " bytes from client " << clientFd << std::endl;

    if (bytes_read < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            // Using error handling system for read errors
            sendErrorResponse(clientFd, HTTP::StatusCode::INTERNAL_SERVER_ERROR);
            std::cerr << "Failed to read from client socket: " << strerror(errno) << std::endl;
            return;
        }
        return;
    }
    
    if (bytes_read == 0) {
        std::cout << "Client " << clientFd << " disconnected." << std::endl;
        disconnectClient(clientFd);
        return;
    }

    // Ensure null termination
    buffer[bytes_read] = '\0';
    
    // Append to the incoming data buffer
    _incomingData[clientFd].append(buffer, bytes_read);
    
    // Check if we have a complete HTTP request
    if (_incomingData[clientFd].find("\r\n\r\n") != std::string::npos) {
        std::cout << "Complete HTTP request received" << std::endl;
        
        try {
            // Use the HTTPHandler to process the request
            auto response = _httpHandler.handleRequest(_incomingData[clientFd]);
            
            // Store the response for sending
            _outgoingData[clientFd] = response->generateResponse();
            
            // Switch to write mode
            _connectionManager.removeConnection(clientFd);
            _connectionManager.addConnection(clientFd, POLLOUT);
        } catch (const std::exception& e) {
            // Use the error handling system for any exceptions during request processing
            std::cerr << "Error processing request: " << e.what() << std::endl;
            sendErrorResponse(clientFd, HTTP::StatusCode::INTERNAL_SERVER_ERROR);
        }
        
        // Clear the incoming data buffer
        _incomingData[clientFd].clear();
    }
}

void ClientHandler::handleWrite(int clientFd) {
    if (_outgoingData.find(clientFd) == _outgoingData.end() || _outgoingData[clientFd].empty())
        return;

    std::string& data = _outgoingData[clientFd];

    ssize_t bytes_written = write(clientFd, data.c_str(), data.size());

    if (bytes_written < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Failed to write to client socket: " << strerror(errno) << std::endl;
            sendErrorResponse(clientFd, HTTP::StatusCode::INTERNAL_SERVER_ERROR);
            return;
        }
        return;
    }
    if (bytes_written == 0) {
        std::cout << "Client " << clientFd << " disconnected." << std::endl;
        disconnectClient(clientFd);
        return;
    }

    data.erase(0, bytes_written);
    _connectionManager.removeConnection(clientFd); 
    if (!data.empty())
        _connectionManager.addConnection(clientFd, POLLOUT);
    else {
        _outgoingData.erase(clientFd);
        _connectionManager.addConnection(clientFd, POLLIN);
        std::cout << "Data sent to client " << clientFd << std::endl;
    }
}

void ClientHandler::handleError(int clientFd) {
    std::cerr << "Error occurred with client " << clientFd << std::endl;
    disconnectClient(clientFd);
}

void ClientHandler::handleTimeout(int clientFd) {
    std::cout << "Connection timeout for client " << clientFd << std::endl;
    disconnectClient(clientFd);
}

// Helper method for sending error responses
void ClientHandler::sendErrorResponse(int clientFd, HTTP::StatusCode status) {
    // Use HTTPHandler's error handler instead of directly using ResourceHandler
    auto errorResponse = _httpHandler.handleError(status);
    std::string responseStr = errorResponse->generateResponse();
    _outgoingData[clientFd] = responseStr;
    
    // Switch socket to write mode
    _connectionManager.removeConnection(clientFd);
    _connectionManager.addConnection(clientFd, POLLOUT);
}