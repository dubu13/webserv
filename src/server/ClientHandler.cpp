#include "ClientHandler.hpp"
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <cstring> // For strerror function
#include "Client.hpp"

ClientHandler::ClientHandler(ConnectionManager& connectionManager, const std::string& webRoot, time_t clientTimeout)
    : _connectionManager(connectionManager), _httpHandler(webRoot), _clientTimeout(clientTimeout) {
    // Initialize the event handler map
    _eventHandlers[ClientEventType::READ] = &ClientHandler::handleRead;
    _eventHandlers[ClientEventType::WRITE] = &ClientHandler::handleWrite;
    _eventHandlers[ClientEventType::ERROR] = &ClientHandler::handleError;
    _eventHandlers[ClientEventType::TIMEOUT] = &ClientHandler::handleTimeout;
    
    // Get reference to the ResourceHandler from HTTPHandler
    _resourceHandler = &_httpHandler.getResourceHandler();
}

ClientHandler::~ClientHandler() {
    // Close all client connections
    std::map<int, Client>::iterator it;
    for (it = _clients.begin(); it != _clients.end(); ++it) {
        int fd = it->first;
        try {
            if (close(fd) < 0) {
                std::cerr << "Failed to close client socket: " << strerror(errno) << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error closing client socket: " << e.what() << std::endl;
        }
    }
    _clients.clear();
}

void ClientHandler::addClient(int clientFd, const struct sockaddr_in& clientAddr) {
    std::cout << "DEBUG: Starting addClient(" << clientFd << ")" << std::endl;
    
    // Create new client and add to map
    std::pair<std::map<int, Client>::iterator, bool> result = 
        _clients.insert(std::make_pair(clientFd, Client(clientFd, clientAddr)));
    
    std::cout << "DEBUG: Client inserted into map" << std::endl;
    
    // Add new client to poll monitoring
    _connectionManager.addConnection(clientFd, POLLIN);
    
    std::cout << "DEBUG: Client added to connection manager" << std::endl;
    
    // Use the iterator from insert to access the client safely
    Client& client = result.first->second;
    std::cout << "New connection from " << client.getIpAddress() << ":" 
              << client.getPort() << " fd: " << clientFd << std::endl;
    
    std::cout << "DEBUG: addClient() completed successfully" << std::endl;
}

void ClientHandler::disconnectClient(int clientFd) {
    // Remove from connection manager first
    _connectionManager.removeConnection(clientFd);
    
    // Find the client in the map
    std::map<int, Client>::iterator it = _clients.find(clientFd);
    if (it == _clients.end()) {
        throw std::runtime_error("Attempted to disconnect non-existent client");
    }
    
    // Close the socket
    if (close(clientFd) < 0) {
        throw std::runtime_error("Failed to close client socket: " + std::string(strerror(errno)));
    }
    
    // Remove from client map
    _clients.erase(it);
    
    std::cout << "Client disconnected, fd: " << clientFd << std::endl;
}

bool ClientHandler::hasClient(int clientFd) const {
    return _clients.find(clientFd) != _clients.end();
}

const std::unordered_set<int>& ClientHandler::getClients() const {
    return _clientFds;
}

void ClientHandler::checkTimeouts() {
    // Create a list of clients to disconnect to avoid modifying the map during iteration
    std::vector<int> timeoutFds;
    
    std::map<int, Client>::const_iterator it;
    for (it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second.hasTimedOut(_clientTimeout)) {
            timeoutFds.push_back(it->first);
        }
    }
    
    // Disconnect timed out clients
    for (size_t i = 0; i < timeoutFds.size(); ++i) {
        handleClientEvent(ClientEventType::TIMEOUT, timeoutFds[i]);
    }
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
    std::map<int, Client>::iterator it = _clients.find(clientFd);
    if (it == _clients.end()) {
        std::cerr << "Attempted to read from non-existent client: " << clientFd << std::endl;
        return;
    }
    
    Client& client = it->second;
    ssize_t bytes_read = client.readData();
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

    // Check if we have a complete HTTP request
    if (client.hasCompleteRequest()) {
        std::cout << "Complete HTTP request received" << std::endl;
        
        try {
            // Use the HTTPHandler to process the request
            auto response = _httpHandler.handleRequest(client.getIncomingData());
            
            // Store the response for sending
            client.setOutgoingData(response->generateResponse());
            
            // Switch to write mode
            _connectionManager.removeConnection(clientFd);
            _connectionManager.addConnection(clientFd, POLLOUT);
        } catch (const std::exception& e) {
            // Use the error handling system for any exceptions during request processing
            std::cerr << "Error processing request: " << e.what() << std::endl;
            sendErrorResponse(clientFd, HTTP::StatusCode::INTERNAL_SERVER_ERROR);
        }
        
        // Clear the incoming data buffer
        client.clearIncomingData();
    }
}

void ClientHandler::handleWrite(int clientFd) {
    std::map<int, Client>::iterator it = _clients.find(clientFd);
    if (it == _clients.end()) {
        std::cerr << "Attempted to write to non-existent client: " << clientFd << std::endl;
        return;
    }
    
    Client& client = it->second;
    
    if (!client.hasDataToWrite()) {
        return;
    }

    ssize_t bytes_written = client.writeData();

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

    if (client.hasDataToWrite()) {
        // Still has data to write, make sure we're in write mode
        _connectionManager.removeConnection(clientFd);
        _connectionManager.addConnection(clientFd, POLLOUT);
    } else {
        // All data sent, switch back to read mode for next request
        _connectionManager.removeConnection(clientFd);
        _connectionManager.addConnection(clientFd, POLLIN);
        std::cout << "Data sent to client " << clientFd << std::endl;
        
        // If the client didn't request keep-alive, disconnect
        if (!client.isKeepAlive()) {
            disconnectClient(clientFd);
        }
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
    std::map<int, Client>::iterator it = _clients.find(clientFd);
    if (it == _clients.end()) {
        std::cerr << "Attempted to send error to non-existent client: " << clientFd << std::endl;
        return;
    }
    
    // Use HTTPHandler's error handler
    auto errorResponse = _httpHandler.handleError(status);
    std::string responseStr = errorResponse->generateResponse();
    
    // Set the response in the client
    it->second.setOutgoingData(responseStr);
    
    // Switch socket to write mode
    _connectionManager.removeConnection(clientFd);
    _connectionManager.addConnection(clientFd, POLLOUT);
}