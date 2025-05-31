#include "ClientHandler.hpp"
#include "Server.hpp"
#include "HTTP/HttpResponseBuilder.hpp"
#include "utils/Logger.hpp"
#include <cerrno>
#include <cstring>
#include <iostream>
#include <unistd.h>

ClientHandler::ClientHandler(Server &server, const std::string &webRoot, time_t timeout)
    : _server(server), _httpHandler(webRoot, &server.getConfig()), _timeout(timeout) {}

ClientHandler::~ClientHandler() {
  // Client objects will automatically clean up their sockets via RAII
}

bool ClientHandler::hasCompleteRequest(const std::string &data) const {
  size_t pos = data.find("\r\n\r\n");
  if (pos == std::string::npos) return false;
  
  size_t contentLengthPos = data.find("Content-Length:");
  if (contentLengthPos != std::string::npos && contentLengthPos < pos) {
    size_t valueStart = data.find(":", contentLengthPos) + 1;
    size_t valueEnd = data.find("\r\n", valueStart);
    if (valueEnd != std::string::npos) {
      std::string lengthStr = data.substr(valueStart, valueEnd - valueStart);
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
  
  return true;
}

ssize_t ClientHandler::readClientData(int fd, server::Client &client, size_t maxSize) {
  char buffer[8192];
  ssize_t bytesRead = recv(fd, buffer, sizeof(buffer), MSG_DONTWAIT);
  
  Logger::debugf("recv() on fd %d returned %zd bytes", fd, bytesRead);
  
  if (bytesRead > 0) {
    std::string& incomingData = client.getIncomingData();
    if (incomingData.length() + bytesRead > maxSize) {
      Logger::warnf("Payload too large: current=%zu, incoming=%zd, max=%zu", 
                    incomingData.length(), bytesRead, maxSize);
      return -2;
    }
    incomingData.append(buffer, bytesRead);
    Logger::debugf("Appended %zd bytes to client data (total: %zu bytes)", bytesRead, incomingData.length());
  } else if (bytesRead == 0) {
    Logger::debugf("Connection closed by client (fd: %d)", fd);
    return 0;
  } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
    Logger::debugf("recv() error on fd %d: %s (errno: %d)", fd, strerror(errno), errno);
    return -1;
  } else {
    Logger::debugf("recv() would block on fd %d", fd);
  }
  
  return bytesRead;
}

ssize_t ClientHandler::writeClientData(int fd, server::Client &client) {
  std::string& outgoingData = client.getOutgoingData();
  if (outgoingData.empty()) {
    Logger::debugf("No outgoing data to write for fd: %d", fd);
    return 0;
  }
  
  size_t bytesToSend = outgoingData.length();
  ssize_t bytesSent = send(fd, outgoingData.c_str(), bytesToSend, MSG_DONTWAIT);
  
  Logger::debugf("send() on fd %d: attempted=%zu, sent=%zd", fd, bytesToSend, bytesSent);
  
  if (bytesSent > 0) {
    outgoingData.erase(0, bytesSent);
    Logger::debugf("Sent %zd bytes, %zu bytes remaining in buffer", bytesSent, outgoingData.length());
  } else if (bytesSent == 0) {
    Logger::debugf("Connection closed during write (fd: %d)", fd);
    return 0;
  } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
    Logger::debugf("send() error on fd %d: %s (errno: %d)", fd, strerror(errno), errno);
    return -1;
  } else {
    Logger::debugf("send() would block on fd %d", fd);
  }
  
  return bytesSent;
}

void ClientHandler::addClient(int fd, const struct sockaddr_in &addr) {
  auto result = _clients.emplace(fd, server::Client(fd, addr));
  _server.getPoller().addFd(fd, POLLIN);
  server::Client &client = result.first->second;
  Logger::infof("Client connected: %s:%d (fd: %d)", 
                client.getIpAddress().c_str(), client.getPort(), fd);
}

void ClientHandler::removeClient(int fd) {
  auto it = _clients.find(fd);
  if (it != _clients.end()) {
    _server.getPoller().removeFd(fd);
    // Client destructor will automatically close the socket via RAII
    _clients.erase(it);
    Logger::infof("Client disconnected (fd: %d)", fd);
  }
}

bool ClientHandler::hasClient(int fd) const {
  return _clients.find(fd) != _clients.end();
}

size_t ClientHandler::getClientCount() const { 
  return _clients.size(); 
}

void ClientHandler::handleRead(int fd) {
  Logger::debugf("Handling read for fd: %d", fd);
  auto it = _clients.find(fd);
  if (it == _clients.end()) {
    Logger::warnf("Read event for unknown client fd: %d", fd);
    return;
  }
  
  server::Client &client = it->second;
  size_t maxBodySize = _server.getConfig().clientMaxBodySize;
  
  Logger::debugf("Reading from client fd: %d (max body size: %zu)", fd, maxBodySize);
  ssize_t bytes = readClientData(fd, client, maxBodySize);
  
  if (bytes < 0) {
    if (bytes == -2) {
      Logger::warnf("Payload too large for client fd: %d", fd);
      std::string errorResponse = HttpResponseBuilder::createErrorResponse(HTTP::StatusCode::PAYLOAD_TOO_LARGE);
      client.getOutgoingData() = errorResponse;
      _server.getPoller().setFdEvents(fd, POLLOUT);
      return;
    } else if (bytes == -1) {
      Logger::errorf("Read error for client fd: %d", fd);
      std::string errorResponse = HttpResponseBuilder::createErrorResponse(HTTP::StatusCode::INTERNAL_SERVER_ERROR);
      client.getOutgoingData() = errorResponse;
      _server.getPoller().setFdEvents(fd, POLLOUT);
      return;
    }
  } else if (bytes == 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
    Logger::infof("Client disconnected during read (fd: %d)", fd);
    removeClient(fd);
    return;
  }
  
  if (bytes > 0) {
    client.updateActivity();
    Logger::debugf("Updated activity timestamp for client fd: %d", fd);
  }
  
  if (hasCompleteRequest(client.getIncomingData())) {
    Logger::debugf("Complete HTTP request received from fd: %d", fd);
    processRequest(fd, client);
  } else if (client.hasDataToWrite()) {
    Logger::debugf("Setting fd %d for both read and write events", fd);
    _server.getPoller().setFdEvents(fd, POLLIN | POLLOUT);
  }
}

void ClientHandler::handleWrite(int fd) {
  Logger::debugf("Handling write for fd: %d", fd);
  auto it = _clients.find(fd);
  if (it == _clients.end()) {
    Logger::warnf("Write event for unknown client fd: %d", fd);
    return;
  }
  
  server::Client &client = it->second;
  
  if (!client.hasDataToWrite()) {
    Logger::debugf("No data to write for fd: %d, switching to read-only mode", fd);
    _server.getPoller().setFdEvents(fd, POLLIN);
    if (!client.isKeepAlive()) {
      Logger::debugf("Connection not keep-alive, removing client fd: %d", fd);
      removeClient(fd);
    }
    return;
  }
  
  Logger::debugf("Writing %zu bytes to client fd: %d", client.getOutgoingData().length(), fd);
  ssize_t bytes = writeClientData(fd, client);
  
  if (bytes <= 0) {
    if (bytes == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
      Logger::infof("Write failed or connection closed for fd: %d", fd);
      removeClient(fd);
    }
    return;
  }
  
  if (bytes > 0) {
    client.updateActivity();
    Logger::debugf("Updated activity timestamp for client fd: %d after write", fd);
  }
  
  if (!client.hasDataToWrite()) {
    Logger::debugf("All data written for fd: %d, switching to read-only mode", fd);
    _server.getPoller().setFdEvents(fd, POLLIN);
    if (!client.isKeepAlive()) {
      Logger::debugf("Connection not keep-alive, removing client fd: %d", fd);
      removeClient(fd);
    }
  }
}

void ClientHandler::processRequest(int fd, server::Client &client) {
  Logger::debugf("Processing HTTP request for fd: %d", fd);
  Logger::debugf("Request data length: %zu bytes", client.getIncomingData().length());
  
  // Log first line of request for debugging
  size_t firstLineEnd = client.getIncomingData().find("\r\n");
  if (firstLineEnd != std::string::npos) {
    std::string requestLine = client.getIncomingData().substr(0, firstLineEnd);
    Logger::infof("Request line: %s", requestLine.c_str());
  }
  
  try {
    std::string response = _httpHandler.handleRequest(client.getIncomingData());
    client.getOutgoingData() = response;
    client.clearIncomingData();
    Logger::debugf("Generated response (%zu bytes) for fd: %d", response.length(), fd);
    _server.getPoller().setFdEvents(fd, POLLOUT);
  } catch (const std::exception &e) {
    Logger::errorf("Request processing error for fd %d: %s", fd, e.what());
    sendError(fd, 500);
  }
}

void ClientHandler::sendError(int fd, int status) {
  auto it = _clients.find(fd);
  if (it == _clients.end()) return;
  
  try {
    std::string response = HttpResponseBuilder::createErrorResponse(static_cast<HTTP::StatusCode>(status));
    it->second.getOutgoingData() = response;
    _server.getPoller().setFdEvents(fd, POLLOUT);
  } catch (const std::exception &e) {
    removeClient(fd);
  }
}

void ClientHandler::checkTimeouts() {
  Logger::debugf("ClientHandler::checkTimeouts - Checking timeouts for %zu clients", _clients.size());
  
  std::vector<int> timedOut;
  time_t currentTime = time(NULL);
  size_t activeConnections = 0;
  
  for (const auto &pair : _clients) {
    const server::Client &client = pair.second;
    time_t idleTime = currentTime - client.getLastActivity();
    time_t timeoutLimit = _timeout;
    
    if (client.hasDataToWrite()) {
      timeoutLimit = _timeout / 2;
      activeConnections++;
      Logger::debugf("Client fd %d has pending data, using reduced timeout: %ld seconds", pair.first, timeoutLimit);
    }
    
    if (idleTime > timeoutLimit) {
      Logger::warnf("Client fd %d timed out (idle: %ld seconds, limit: %ld seconds)", 
                    pair.first, idleTime, timeoutLimit);
      timedOut.push_back(pair.first);
    } else {
      Logger::debugf("Client fd %d is active (idle: %ld seconds < limit: %ld seconds)", 
                     pair.first, idleTime, timeoutLimit);
    }
  }
  
  Logger::debugf("Timeout check complete: %zu clients checked, %zu active with pending data, %zu timed out", 
                 _clients.size(), activeConnections, timedOut.size());
  
  for (int fd : timedOut) {
    Logger::infof("Removing timed out client (fd: %d)", fd);
    removeClient(fd);
  }
}
