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
#include <atomic>

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
extern std::atomic<bool> g_running;

Server::Server(const ServerBlock* config)
    : _serverFd(-1)
    , _running(false)
    , _config(config)
    , _router(config) {

    ErrorResponseBuilder::setCurrentConfig(config);
}

Server::~Server() {
    if (_serverFd >= 0) {
        close(_serverFd);
    }
}

// bool Server::start() {
//     if (_serverFd < 0)
//         setupSocket();

//     if (_serverFd < 0)
//         return false;

//     _running = true;

//     while (_running && g_running) {
//         auto activeFds = _poller.poll(1000);

//         for (const auto& pfd : activeFds) {
            
//             if (pfd.fd == _serverFd) {
//                 acceptConnection();
//             } else
//                 handleClient(pfd.fd);
//         }
//         checkTimeouts();
//     }
//     return true;
// }

int Server::setupSocket() {
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0) {
        Logger::error("Failed to create socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        Logger::error("Failed to set socket options");
        close(_serverFd);
        _serverFd = -1;
        return -1;
    }

    int flags = fcntl(_serverFd, F_GETFL, 0);
    if (flags < 0 || fcntl(_serverFd, F_SETFL, flags | O_NONBLOCK) < 0) {
        Logger::error("Failed to set non-blocking mode");
        close(_serverFd);
        _serverFd = -1;
        return -1;
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
        return -1;
    }

    if (listen(_serverFd, SOMAXCONN) < 0) {
        Logger::error("Failed to listen on socket");
        close(_serverFd);
        _serverFd = -1;
        return -1;
    }    
    return _serverFd;
}

int Server::acceptConnection() {
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    int clientFd = accept(_serverFd, (struct sockaddr*)&clientAddr, &addrLen);
    if (clientFd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            Logger::error("Failed to accept connection");
        }
        return -1;
    }

    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
        Logger::error("Failed to set client socket to non-blocking mode");
        close(clientFd);
        return -1;
    }

    _clients[clientFd] = std::time(nullptr);
    _clientBuffers[clientFd] = "";
    
    Logger::logf<LogLevel::INFO>("New client connected: fd=%d", clientFd);
    return clientFd;
}

void Server::handleClient(int fd) {
    char buffer[4096];
    ssize_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        if (bytesRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return;
        }
        removeClient(fd);
        return;
    }

    buffer[bytesRead] = '\0';

    _clientBuffers[fd] += std::string(buffer, bytesRead);

    _clients[fd] = std::time(nullptr);

    if (!HttpUtils::isCompleteRequest(_clientBuffers[fd])) {
        return;
    }

    try {
        Request request;
        if (!parseRequest(_clientBuffers[fd], request)) {

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

        if (responseStr.find("Connection:") == std::string::npos) {
            size_t headerEnd = responseStr.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                responseStr.insert(headerEnd, "\r\nConnection: close");
            }
        }

        send(fd, responseStr.c_str(), responseStr.length(), MSG_NOSIGNAL);
        removeClient(fd);

    } catch (const std::exception& e) {
        Logger::logf<LogLevel::ERROR>("Error handling client: %s", e.what());
        std::string errorResponse = ErrorResponseBuilder::buildResponse(500);
        send(fd, errorResponse.c_str(), errorResponse.length(), MSG_NOSIGNAL);
        removeClient(fd);
    }
}

void Server::removeClient(int fd) {
    // Note: Don't remove from poller here - ServerManager handles it
    _clients.erase(fd);
    _clientBuffers.erase(fd);
    close(fd);
    Logger::logf<LogLevel::WARN>("Client removed: fd=%d", fd);
}

void Server::checkTimeouts() {
    const time_t timeout = 30;
    const time_t now = std::time(nullptr);

    auto it = _clients.begin();
    while (it != _clients.end()) {
        if (now - it->second > timeout) {
            int fd = it->first;

            std::string timeoutResponse = ErrorResponseBuilder::buildResponse(408);
            send(fd, timeoutResponse.c_str(), timeoutResponse.length(), MSG_NOSIGNAL);

            it = _clients.erase(it);
            _clientBuffers.erase(fd);
            // Note: Don't remove from poller here - ServerManager handles it
            close(fd);
        } else {
            ++it;
        }
    }
}
