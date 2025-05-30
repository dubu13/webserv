#include "ClientHandler.hpp"
#include "Server.hpp"
#include <cerrno>
#include <cstring>
#include <iostream>
#include <unistd.h>

ClientHandler::ClientHandler(Server &server, const std::string &webRoot, time_t timeout)
    : _server(server), _httpHandler(webRoot, &server.getConfig()), _timeout(timeout) {}

ClientHandler::~ClientHandler() {
  for (auto &pair : _clients) {
    close(pair.first);
  }
}

bool ClientHandler::hasCompleteRequest(const std::string &data) const {
  size_t pos = data.find("\r\n\r\n");
  if (pos == std::string::npos) return false;
  
  // Check if it's a POST request with Content-Length
  size_t contentLengthPos = data.find("Content-Length:");
  if (contentLengthPos != std::string::npos && contentLengthPos < pos) {
    size_t valueStart = data.find(":", contentLengthPos) + 1;
    size_t valueEnd = data.find("\r\n", valueStart);
    if (valueEnd != std::string::npos) {
      std::string lengthStr = data.substr(valueStart, valueEnd - valueStart);
      // Trim whitespace
      lengthStr.erase(0, lengthStr.find_first_not_of(" \t"));
      lengthStr.erase(lengthStr.find_last_not_of(" \t") + 1);
      
      try {
        size_t contentLength = std::stoul(lengthStr);
        size_t headerEnd = pos + 4;
        return data.length() >= headerEnd + contentLength;
      } catch (...) {
        return false;
      }
    }
  }
  
  return true; // GET requests and others without body
}

ssize_t ClientHandler::readClientData(int fd, std::string &incomingData, size_t maxSize) {
  char buffer[8192];
  ssize_t bytesRead = recv(fd, buffer, sizeof(buffer), MSG_DONTWAIT);
  
  if (bytesRead > 0) {
    if (incomingData.length() + bytesRead > maxSize) {
      return -2; // Payload too large
    }
    incomingData.append(buffer, bytesRead);
  } else if (bytesRead == 0) {
    return 0; // Connection closed
  } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
    return -1; // Error
  }
  
  return bytesRead;
}

ssize_t ClientHandler::writeClientData(int fd, std::string &outgoingData) {
  if (outgoingData.empty()) return 0;
  
  ssize_t bytesSent = send(fd, outgoingData.c_str(), outgoingData.length(), MSG_DONTWAIT);
  
  if (bytesSent > 0) {
    outgoingData.erase(0, bytesSent);
  } else if (bytesSent == 0) {
    return 0; // Connection closed
  } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
    return -1; // Error
  }
  
  return bytesSent;
}

void ClientHandler::addClient(int fd, const struct sockaddr_in &addr) {
  auto result = _clients.emplace(fd, ClientData(fd, addr));
  _server.getPoller().addFd(fd, POLLIN);
  ClientData &client = result.first->second;
  std::cout << "Client connected: " << client.getIpAddress() << ":" 
            << client.getPort() << " (fd: " << fd << ")" << std::endl;
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
  
  ClientData &client = it->second;
  size_t maxBodySize = _server.getConfig().client_max_body_size;
  
  ssize_t bytes = readClientData(fd, client.incomingData, maxBodySize);
  
  if (bytes < 0) {
    if (bytes == -2) {
      std::string errorResponse = HTTP::createErrorResponse(HTTP::StatusCode::PAYLOAD_TOO_LARGE);
      client.outgoingData = errorResponse;
      _server.getPoller().setFdEvents(fd, POLLOUT);
      return;
    } else if (bytes == -1) {
      std::string errorResponse = HTTP::createErrorResponse(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
      client.outgoingData = errorResponse;
      _server.getPoller().setFdEvents(fd, POLLOUT);
      return;
    }
  } else if (bytes == 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
    removeClient(fd);
    return;
  }
  
  if (bytes > 0) {
    client.updateActivity();
  }
  
  if (hasCompleteRequest(client.incomingData)) {
    processRequest(fd, client);
  } else if (client.hasDataToWrite()) {
    _server.getPoller().setFdEvents(fd, POLLIN | POLLOUT);
  }
}
void ClientHandler::handleWrite(int fd) {
  auto it = _clients.find(fd);
  if (it == _clients.end()) return;
  
  ClientData &client = it->second;
  
  if (!client.hasDataToWrite()) {
    _server.getPoller().setFdEvents(fd, POLLIN);
    if (!client.keepAlive) {
      removeClient(fd);
    }
    return;
  }
  
  ssize_t bytes = writeClientData(fd, client.outgoingData);
  
  if (bytes <= 0) {
    if (bytes == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
      removeClient(fd);
    }
    return;
  }
  
  if (bytes > 0) {
    client.updateActivity();
  }
  
  if (!client.hasDataToWrite()) {
    _server.getPoller().setFdEvents(fd, POLLIN);
    if (!client.keepAlive) {
      removeClient(fd);
    }
  }
}

void ClientHandler::processRequest(int fd, ClientData &client) {
  try {
    std::string response = _httpHandler.handleRequest(client.incomingData);
    client.outgoingData = response;
    client.incomingData.clear();
    _server.getPoller().setFdEvents(fd, POLLOUT);
  } catch (const std::exception &e) {
    std::cerr << "Request processing error: " << e.what() << std::endl;
    sendError(fd, 500);
  }
}

void ClientHandler::sendError(int fd, int status) {
  auto it = _clients.find(fd);
  if (it == _clients.end()) return;
  
  try {
    std::string response = HTTP::createErrorResponse(static_cast<HTTP::StatusCode>(status));
    it->second.outgoingData = response;
    _server.getPoller().setFdEvents(fd, POLLOUT);
  } catch (const std::exception &e) {
    removeClient(fd);
  }
}

void ClientHandler::checkTimeouts() {
  std::vector<int> timedOut;
  time_t currentTime = time(NULL);
  
  for (const auto &pair : _clients) {
    const ClientData &client = pair.second;
    time_t idleTime = currentTime - client.lastActivity;
    time_t timeoutLimit = _timeout;
    
    if (client.hasDataToWrite()) {
      timeoutLimit = _timeout / 2;
    }
    
    if (idleTime > timeoutLimit) {
      timedOut.push_back(pair.first);
    }
  }
  
  for (int fd : timedOut) {
    std::cout << "Client timeout (fd: " << fd << ")" << std::endl;
    removeClient(fd);
  }
}
