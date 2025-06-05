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
#include "utils/Utils.hpp"
#include <sstream>

using HTTP::parseRequest;
using HTTP::Request;

Server::Server(const ServerBlock* config)
    : _serverFd(-1)
    , _running(false)
    , _config(config)
    , _router(config) {}

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
            return;
        }
        removeClient(fd);
        return;
    }

    buffer[bytesRead] = '\0';
    std::string requestData(buffer, bytesRead);

    try {
        Request request;
        if (!parseRequest(requestData, request)) {
            Logger::error("Failed to parse HTTP request");
            removeClient(fd);
            return;
        }

        Logger::debugf("Processing request URI: '%s'", request.requestLine.uri.c_str());

        std::string root = "./www";
        if (_config && !_config->root.empty()) {
            root = _config->root;
        }

        std::string responseStr = MethodHandler::handleRequest(request, root);
        std::string responseStr = MethodHandler::handleRequest(request, root, &_router);
        send(fd, responseStr.c_str(), responseStr.length(), 0);

        removeClient(fd);

    } catch (const std::exception& e) {
        Logger::errorf("Error handling client: %s", e.what());
        removeClient(fd);
    }
}

void Server::removeClient(int fd) {
    _poller.remove(fd);
    _clients.erase(fd);
    close(fd);
}

void Server::checkTimeouts() {
    const time_t timeout = 60;
    const time_t now = std::time(nullptr);

    auto it = _clients.begin();
    while (it != _clients.end()) {
        if (now - it->second > timeout) {
            int fd = it->first;
            it = _clients.erase(it);
            _poller.remove(fd);
            close(fd);
        } else {
            ++it;
        }
    }
}
