#include "ClientHandler.hpp"
#include "Server.hpp"
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <cstring>
#include "Client.hpp"
ClientHandler::ClientHandler(Server& server, const std::string& webRoot, time_t clientTimeout)
    : _server(server), _httpHandler(webRoot), _clientTimeout(clientTimeout) {
}
ClientHandler::~ClientHandler() {
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
    std::pair<std::map<int, Client>::iterator, bool> result =
        _clients.insert(std::make_pair(clientFd, Client(clientFd, clientAddr)));
    _server.addConnection(clientFd, POLLIN);
    Client& client = result.first->second;
    std::cout << "New connection from " << client.getIpAddress() << ":"
              << client.getPort() << " fd: " << clientFd << std::endl;
}
void ClientHandler::disconnectClient(int clientFd) {
    _server.removeConnection(clientFd);
    std::map<int, Client>::iterator it = _clients.find(clientFd);
    if (it == _clients.end()) {
        throw std::runtime_error("Attempted to disconnect non-existent client");
    }
    if (close(clientFd) < 0) {
        throw std::runtime_error("Failed to close client socket: " + std::string(strerror(errno)));
    }
    _clients.erase(it);
}
bool ClientHandler::hasClient(int clientFd) const {
    return _clients.find(clientFd) != _clients.end();
}
size_t ClientHandler::getClientCount() const {
    return _clients.size();
}
std::vector<int> ClientHandler::getAllClientFds() const {
    std::vector<int> fds;
    for (std::map<int, Client>::const_iterator it = _clients.begin(); it != _clients.end(); ++it) {
        fds.push_back(it->first);
    }
    return fds;
}
void ClientHandler::checkTimeouts() {
    std::vector<int> timeoutFds;
    std::map<int, Client>::const_iterator it;
    for (it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second.hasTimedOut(_clientTimeout)) {
            timeoutFds.push_back(it->first);
        }
    }
    for (size_t i = 0; i < timeoutFds.size(); ++i) {
        handleClientEvent(ClientEventType::TIMEOUT, timeoutFds[i]);
    }
}
void ClientHandler::handleClientEvent(ClientEventType eventType, int clientFd) {
    switch (eventType) {
        case ClientEventType::READ:
            handleRead(clientFd);
            break;
        case ClientEventType::WRITE:
            handleWrite(clientFd);
            break;
        case ClientEventType::ERROR:
            handleError(clientFd);
            break;
        case ClientEventType::TIMEOUT:
            handleTimeout(clientFd);
            break;
        default:
            std::cerr << "Unknown event type for client " << clientFd << std::endl;
            break;
    }
}
void ClientHandler::handleRead(int clientFd) {
    std::map<int, Client>::iterator it = _clients.find(clientFd);
    if (it == _clients.end()) {
        std::cerr << "Attempted to read from non-existent client: " << clientFd << std::endl;
        return;
    }
    Client& client = it->second;
    ssize_t bytes_read = client.readData();
    if (!processReadResult(clientFd, client, bytes_read)) {
        return;
    }
    if (client.hasCompleteRequest()) {
        processCompleteRequest(clientFd, client);
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
    if (!processWriteResult(clientFd, client, bytes_written)) {
        return;
    }
    handleWriteCompletion(clientFd, client);
}
void ClientHandler::handleError(int clientFd) {
    std::cerr << "Error occurred with client " << clientFd << std::endl;
    disconnectClient(clientFd);
}
void ClientHandler::handleTimeout(int clientFd) {
    disconnectClient(clientFd);
}
void ClientHandler::sendErrorResponse(int clientFd, HTTP::StatusCode status) {
    std::map<int, Client>::iterator it = _clients.find(clientFd);
    if (it == _clients.end()) {
        std::cerr << "Attempted to send error to non-existent client: " << clientFd << std::endl;
        return;
    }
    auto errorResponse = _httpHandler.handleError(status);
    std::string responseStr = errorResponse->generateResponse();
    it->second.setOutgoingData(responseStr);
    _server.removeConnection(clientFd);
    _server.addConnection(clientFd, POLLOUT);
}
bool ClientHandler::processReadResult(int clientFd, Client& client, ssize_t bytes_read) {
    (void)client;
    if (bytes_read < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            sendErrorResponse(clientFd, HTTP::StatusCode::INTERNAL_SERVER_ERROR);
            std::cerr << "Failed to read from client socket: " << strerror(errno) << std::endl;
        }
        return false;
    }
    if (bytes_read == 0) {
        disconnectClient(clientFd);
        return false;
    }
    return true;
}
void ClientHandler::processCompleteRequest(int clientFd, Client& client) {
    try {
        auto response = _httpHandler.handleRequest(client.getIncomingData());
        client.setOutgoingData(response->generateResponse());
        _server.removeConnection(clientFd);
        _server.addConnection(clientFd, POLLOUT);
    } catch (const std::exception& e) {
        std::cerr << "Error processing request: " << e.what() << std::endl;
        sendErrorResponse(clientFd, HTTP::StatusCode::INTERNAL_SERVER_ERROR);
    }
    client.clearIncomingData();
}
bool ClientHandler::processWriteResult(int clientFd, Client& client, ssize_t bytes_written) {
    (void)client;
    if (bytes_written < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Failed to write to client socket: " << strerror(errno) << std::endl;
            sendErrorResponse(clientFd, HTTP::StatusCode::INTERNAL_SERVER_ERROR);
        }
        return false;
    }
    if (bytes_written == 0) {
        disconnectClient(clientFd);
        return false;
    }
    return true;
}
void ClientHandler::handleWriteCompletion(int clientFd, Client& client) {
    if (client.hasDataToWrite()) {
        _server.removeConnection(clientFd);
        _server.addConnection(clientFd, POLLOUT);
    } else {
        _server.removeConnection(clientFd);
        _server.addConnection(clientFd, POLLIN);
        if (!client.isKeepAlive()) {
            disconnectClient(clientFd);
        }
    }
}