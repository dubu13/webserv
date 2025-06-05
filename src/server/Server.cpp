#include <fcntl.h>
#include <algorithm>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdexcept>

#include "Server.hpp"
#include "Logger.hpp"
#include "HTTP/routing/RequestRouter.hpp"
#include "HTTP/handlers/MethodDispatcher.hpp"
#include "HTTP/core/HttpResponse.hpp"
#include "HTTP/core/HTTPParser.hpp"
#include "HTTP/core/ErrorResponseBuilder.hpp"
#include "utils/Utils.hpp"
#include <sstream>

using HTTP::parseRequest;
using HTTP::Request;

Server::Server(const ServerBlock* config)
    : _serverFd(-1)
    , _running(false)
    , _config(config)
    , _router(config) {
    // Set the config for ErrorResponseBuilder to use custom error pages
    ErrorResponseBuilder::setCurrentConfig(config);
}

Server::~Server() {
    if (_serverFd >= 0) {
        close(_serverFd);
    }
}

bool Server::start() {
    if (_serverFd < 0) {
        setupSocket();
    }

    if (_serverFd < 0) {
        return false;
    }

    _running = true;

    while (_running) {
        auto activeFds = _poller.poll(1000);

        for (const auto& pfd : activeFds) {
            if (pfd.fd == _serverFd) {
                acceptConnection();
            } else {
                handleClient(pfd.fd);
            }
        }

        checkTimeouts();
    }

    return true;
}

void Server::setupSocket() {
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0) {
        Logger::error("Failed to create socket");
        return;
    }

    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        Logger::error("Failed to set socket options");
        close(_serverFd);
        _serverFd = -1;
        return;
    }

    int flags = fcntl(_serverFd, F_GETFL, 0);
    if (flags < 0 || fcntl(_serverFd, F_SETFL, flags | O_NONBLOCK) < 0) {
        Logger::error("Failed to set non-blocking mode");
        close(_serverFd);
        _serverFd = -1;
        return;
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(_config->listenDirectives.empty() ? 8080 : _config->listenDirectives[0].second);

    if (bind(_serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        Logger::error("Failed to bind socket");
        close(_serverFd);
        _serverFd = -1;
        return;
    }

    if (listen(_serverFd, SOMAXCONN) < 0) {
        Logger::error("Failed to listen on socket");
        close(_serverFd);
        _serverFd = -1;
        return;
    }

    _poller.add(_serverFd, POLLIN);
}

void Server::acceptConnection() {
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    int clientFd = accept(_serverFd, (struct sockaddr*)&clientAddr, &addrLen);
    if (clientFd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            Logger::error("Failed to accept connection");
        }
        return;
    }

    int flags = fcntl(clientFd, F_GETFL, 0);
    if (flags < 0 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) < 0) {
        Logger::error("Failed to set client socket to non-blocking mode");
        close(clientFd);
        return;
    }

    _poller.add(clientFd, POLLIN);
    _clients[clientFd] = std::time(nullptr);
}

void Server::handleClient(int fd) {
    char buffer[4096];
    ssize_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        if (bytesRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return; // Would block, try again later
        }
        removeClient(fd);
        return;
    }

    buffer[bytesRead] = '\0';
    
    // Accumulate data in client buffer
    _clientBuffers[fd] += std::string(buffer, bytesRead);
    
    // Update client activity timestamp
    _clients[fd] = std::time(nullptr);
    
    // Check if we have a complete request
    if (!HttpUtils::isCompleteRequest(_clientBuffers[fd])) {
        return; // Wait for more data
    }

    try {
        Request request;
        if (!parseRequest(_clientBuffers[fd], request)) {
            // Use ErrorResponseBuilder for proper 400 response
            std::string errorResponse = ErrorResponseBuilder::buildResponse(400);
            send(fd, errorResponse.c_str(), errorResponse.length(), MSG_NOSIGNAL);
            removeClient(fd);
            return;
        }

        std::string root = "./www";
        if (_config && !_config->root.empty()) {
            root = _config->root;
        }
        
        std::string responseStr = MethodHandler::handleRequest(request, root, &_router);
        
        // Ensure Connection: close header is added
        if (responseStr.find("Connection:") == std::string::npos) {
            size_t headerEnd = responseStr.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                responseStr.insert(headerEnd, "\r\nConnection: close");
            }
        }
        
        send(fd, responseStr.c_str(), responseStr.length(), MSG_NOSIGNAL);
        removeClient(fd); // Always close after response for now

    } catch (const std::exception& e) {
        Logger::errorf("Error handling client: %s", e.what());
        std::string errorResponse = ErrorResponseBuilder::buildResponse(500);
        send(fd, errorResponse.c_str(), errorResponse.length(), MSG_NOSIGNAL);
        removeClient(fd);
    }
}

void Server::removeClient(int fd) {
    _poller.remove(fd);
    _clients.erase(fd);
    _clientBuffers.erase(fd); // Clean up buffer
    close(fd);
    Logger::warnf("Client removed: fd=%d", fd);
}

void Server::checkTimeouts() {
    const time_t timeout = 30;
    const time_t now = std::time(nullptr);

    auto it = _clients.begin();
    while (it != _clients.end()) {
        if (now - it->second > timeout) {
            int fd = it->first;
            
            // Send proper timeout response before closing
            std::string timeoutResponse = ErrorResponseBuilder::buildResponse(408);
            send(fd, timeoutResponse.c_str(), timeoutResponse.length(), MSG_NOSIGNAL);
            
            it = _clients.erase(it);
            _clientBuffers.erase(fd); // Clean up buffer
            _poller.remove(fd);
            close(fd);
        } else {
            ++it;
        }
    }
}
