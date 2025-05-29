#include "ClientHandler.hpp"
#include "Server.hpp"
#include <unistd.h>
#include <iostream>
#include <cerrno>
#include <cstring>
ClientHandler::ClientHandler(Server& server, const std::string& webRoot, time_t timeout)
    : _server(server), _httpHandler(webRoot, &server.getConfig()), _timeout(timeout) {
}
ClientHandler::~ClientHandler() {
    for (auto& pair : _clients) {
        close(pair.first);
    }
}
void ClientHandler::addClient(int fd, const struct sockaddr_in& addr) {
    auto result = _clients.insert(std::make_pair(fd, Client(fd, addr)));
    _server.getPoller().addFd(fd, POLLIN);
    Client& client = result.first->second;
    std::cout << "Client connected: " << client.getIpAddress()
              << ":" << client.getPort() << " (fd: " << fd << ")" << std::endl;
}
void ClientHandler::removeClient(int fd) {
    auto it = _clients.find(fd);
    if (it != _clients.end()) {
        _server.getPoller().removeFd(fd);
        close(fd);
        _clients.erase(it);
        std::cout << "Client disconnected (fd: " << fd << ")" << std::endl;
    }
}
bool ClientHandler::hasClient(int fd) const {
    return _clients.find(fd) != _clients.end();
}
size_t ClientHandler::getClientCount() const {
    return _clients.size();
}
void ClientHandler::handleRead(int fd) {
    auto it = _clients.find(fd);
    if (it == _clients.end()) return;
    Client& client = it->second;
    ssize_t bytes = client.readData();
    if (bytes <= 0) {
        if (bytes == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
            removeClient(fd);
        }
        return;
    }
    if (client.hasCompleteRequest()) {
        processRequest(fd, client);
    }
}
void ClientHandler::handleWrite(int fd) {
    auto it = _clients.find(fd);
    if (it == _clients.end()) return;
    Client& client = it->second;
    if (!client.hasDataToWrite()) {
        _server.getPoller().updateFd(fd, POLLIN);
        if (!client.isKeepAlive()) {
            removeClient(fd);
        }
        return;
    }
    ssize_t bytes = client.writeData();
    if (bytes <= 0) {
        if (bytes == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
            removeClient(fd);
        }
        return;
    }
    if (!client.hasDataToWrite()) {
        _server.getPoller().updateFd(fd, POLLIN);
        if (!client.isKeepAlive()) {
            removeClient(fd);
        }
    }
}
void ClientHandler::processRequest(int fd, Client& client) {
    try {
        auto response = _httpHandler.handleRequest(client.getIncomingData());
        client.setOutgoingData(response->generateResponse());
        client.clearIncomingData();
        _server.getPoller().updateFd(fd, POLLOUT);
    } catch (const std::exception& e) {
        std::cerr << "Request processing error: " << e.what() << std::endl;
        sendError(fd, 500);
    }
}
void ClientHandler::sendError(int fd, int status) {
    auto it = _clients.find(fd);
    if (it == _clients.end()) return;
    try {
        auto response = _httpHandler.handleError(static_cast<HTTP::StatusCode>(status));
        it->second.setOutgoingData(response->generateResponse());
        _server.getPoller().updateFd(fd, POLLOUT);
    } catch (const std::exception& e) {
        removeClient(fd);
    }
}
void ClientHandler::checkTimeouts() {
    std::vector<int> timedOut;
    for (const auto& pair : _clients) {
        if (pair.second.hasTimedOut(_timeout)) {
            timedOut.push_back(pair.first);
        }
    }
    for (int fd : timedOut) {
        removeClient(fd);
    }
}