#include "Server.hpp"
#include <arpa/inet.h>

// Add these includes
#include "HTTP/IHTTPRequest.hpp"
#include "HTTP/HTTPResponse.hpp"
#include "HTTP/HTTPTypes.hpp"
#include "FileType.hpp"  // Add this at the top with your other includes
#include <fstream>
#include <vector>
#include <algorithm>

Server::Server(int port) : _socket(port), _running(false) {
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
    
    // Add new client to poll monitoring
    _connectionManager.addConnection(client_fd, POLLIN);
    
    char client_ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip)) == NULL)
        throw std::runtime_error("Failed to convert client IP address");
    std::cout << "New connection from " << client_ip << ":" << ntohs(client_addr.sin_port) << " fd: " << client_fd << std::endl;
}

void Server::disconnectClient(int client_fd) {
    // Remove from connection manager first
    _connectionManager.removeConnection(client_fd);
    
    // Clean up data structures
    _incomingData.erase(client_fd);
    _outgoingData.erase(client_fd);
    
    // Remove from client set and close
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
    for (int fd : _clientFds) {
        if (close(fd) < 0)
            throw std::runtime_error("Failed to close client socket");
    }
    _clientFds.clear();
    std::cout << "Server stopped." << std::endl;
}

void Server::handleClientRead(int client_fd) {
    char buffer[4096] = {0}; // Initialize buffer to zeros
    
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    std::cout << "Read " << bytes_read << " bytes from client " << client_fd << std::endl;

    if (bytes_read < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            disconnectClient(client_fd);
            throw std::runtime_error("Failed to read from client socket");
        }
        return;
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
    
    // Check if we have a complete HTTP request
    if (_incomingData[client_fd].find("\r\n\r\n") != std::string::npos) {
        std::cout << "Complete HTTP request received" << std::endl;
        
        try {
            // Parse the request using your IHTTPRequest interface
            auto request = IHTTPRequest::createRequest(_incomingData[client_fd]);
            
            if (request) {
                std::cout << "Processing " << HTTP::methodToString(request->getMethod()) 
                         << " request for URI: " << request->getUri() << std::endl;
                
                // Check method and route request appropriately
                if (request->getMethod() == HTTP::Method::GET) {
                    handleGETRequest(client_fd, *request);
                } else {
                    // Respond with 501 Not Implemented for other methods
                    auto response = HTTPResponse::createResponse();
                    response->setStatus(HTTP::StatusCode::METHOD_NOT_ALLOWED);
                    response->setContentType("text/html");
                    response->setBody("<html><body><h1>Method Not Allowed</h1><p>Only GET is currently supported</p></body></html>");
                    
                    // Store the response for sending
                    _outgoingData[client_fd] = response->generateResponse();
                    
                    // Switch to write mode
                    _connectionManager.removeConnection(client_fd);
                    _connectionManager.addConnection(client_fd, POLLOUT);
                }
            } else {
                // Bad request - couldn't parse
                auto response = HTTPResponse::createResponse();
                response->setStatus(HTTP::StatusCode::BAD_REQUEST);
                response->setContentType("text/html");
                response->setBody("<html><body><h1>400 Bad Request</h1><p>Invalid HTTP request format</p></body></html>");
                
                _outgoingData[client_fd] = response->generateResponse();
                _connectionManager.removeConnection(client_fd);
                _connectionManager.addConnection(client_fd, POLLOUT);
            }
        }
        catch (const std::exception& e) {
            // Server error handling
            auto response = HTTPResponse::createResponse();
            response->setStatus(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
            response->setContentType("text/html");
            response->setBody("<html><body><h1>500 Internal Server Error</h1><p>" + std::string(e.what()) + "</p></body></html>");
            
            _outgoingData[client_fd] = response->generateResponse();
            _connectionManager.removeConnection(client_fd);
            _connectionManager.addConnection(client_fd, POLLOUT);
        }
        
        // Clear the incoming data buffer
        _incomingData[client_fd].clear();
    }
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

// Helper function to check if string ends with a suffix (C++17 compatible)
bool endsWith(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

void Server::handleGETRequest(int client_fd, const IHTTPRequest& request) {
    std::string uri = request.getUri();
    
    // Default to index.html for root path
    if (uri == "/") {
        uri = "/index.html";
    }
    
    // Try to serve file from www directory
    std::string filePath = "./www" + uri;
    
    auto response = HTTPResponse::createResponse();
    
    // Check if this is a CGI request
    if (FileType::isCGI(filePath)) {
        // Will implement CGI execution later
        std::string handler = FileType::getCGIHandler(filePath);
        if (!handler.empty()) {
            // TODO: Handle CGI execution
            // For now, just return not implemented
            response->setStatus(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
            response->setContentType("text/html");
            response->setBody("<html><body><h1>CGI Not Yet Implemented</h1></body></html>");
        } else {
            // CGI handler not found
            response->setStatus(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
            response->setContentType("text/html");
            response->setBody("<html><body><h1>500 Internal Server Error</h1><p>No CGI handler available</p></body></html>");
        }
    } else {
        // Not a CGI file - serve the static file
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        
        if (file.is_open()) {
            // File exists - prepare to send it
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            
            std::vector<char> buffer(size);
            if (file.read(buffer.data(), size)) {
                // Set content type based on file extension
                std::string contentType = FileType::getMimeType(filePath);
                
                response->setStatus(HTTP::StatusCode::OK);
                response->setContentType(contentType);
                response->setBody(std::string(buffer.data(), size));
            } else {
                // Error reading file
                response->setStatus(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
                response->setContentType("text/html");
                response->setBody("<html><body><h1>500 Internal Server Error</h1><p>Error reading file</p></body></html>");
            }
        } else {
            // File not found - return 404
            response->setStatus(HTTP::StatusCode::NOT_FOUND);
            response->setContentType("text/html");
            
            // Try to load custom 404 page
            std::ifstream errorFile("./www/errors/404.html");
            if (errorFile.is_open()) {
                std::string errorContent((std::istreambuf_iterator<char>(errorFile)),
                                        std::istreambuf_iterator<char>());
                response->setBody(errorContent);
            } else {
                // Fallback 404 page
                response->setBody("<html><body><h1>404 Not Found</h1><p>The requested resource was not found on this server</p></body></html>");
            }
        }
    }
    
    // Add common headers
    response->setHeader("Connection", "close");
    
    // Send the response
    _outgoingData[client_fd] = response->generateResponse();
    _connectionManager.removeConnection(client_fd);
    _connectionManager.addConnection(client_fd, POLLOUT);
}
