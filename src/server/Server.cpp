#include <algorithm>
#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

#include "HTTP/core/ErrorResponseBuilder.hpp"
#include "HTTP/core/HTTPParser.hpp"
#include "HTTP/core/HttpResponse.hpp"
#include "HTTP/handlers/MethodDispatcher.hpp"
#include "HTTP/routing/RequestRouter.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "utils/Utils.hpp"
#include <sstream>

using HTTP::parseRequest;
using HTTP::Request;
extern std::atomic<bool> g_running;

Server::Server(const ServerBlock *config)
    : _serverFd(-1), _running(false), _config(config), _router(config) {

  ErrorResponseBuilder::setCurrentConfig(config);
}

Server::~Server() {
  if (_serverFd >= 0)
    close(_serverFd);
}

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

  if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) < 0) {
    Logger::error("Failed to set non-blocking mode");
    close(_serverFd);
    _serverFd = -1;
    return -1;
  }

  struct sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(_config->listenDirectives.empty()
                            ? 8080
                            : _config->listenDirectives[0].second);

  if (bind(_serverFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
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
  int clientFd = accept(_serverFd, (struct sockaddr *)&clientAddr, &addrLen);
  if (clientFd < 0) {
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
    removeClient(fd);
    return;
  }
  buffer[bytesRead] = '\0';
  _clientBuffers[fd] += std::string(buffer, bytesRead);
  _clients[fd] = std::time(nullptr);
  if (!HttpUtils::isCompleteRequest(_clientBuffers[fd]))
    return;
  try {
    Request request;
    auto parseResult = parseRequest(_clientBuffers[fd], request);
    if (!parseResult.success) {
      sendErrorToClient(fd, parseResult.statusCode);
      removeClient(fd);
      return;
    }
    std::string root = "./www";
    if (_config && !_config->root.empty())
      root = _config->root;
    std::string responseStr =
        MethodHandler::handleRequest(request, root, &_router);

    size_t headerEnd = responseStr.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
      std::string headers = responseStr.substr(0, headerEnd);
      if (headers.find("\r\nConnection: ") == std::string::npos &&
          headers.find("Connection: ") != 0) {
        responseStr.insert(headerEnd, "\r\nConnection: close");
      }
    }

    send(fd, responseStr.c_str(), responseStr.length(), MSG_NOSIGNAL);
    removeClient(fd);

  } catch (const std::exception &e) {
    Logger::logf<LogLevel::ERROR>("Error handling client: %s", e.what());
    sendErrorToClient(fd, 500);
  }
}

void Server::removeClient(int fd) {

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
      close(fd);
    } else
      ++it;
  }
}

void Server::sendErrorToClient(int fd, int statusCode) {
  try {
    std::string errorResponse = ErrorResponseBuilder::buildResponse(statusCode);
    send(fd, errorResponse.c_str(), errorResponse.length(), MSG_NOSIGNAL);
  } catch (const std::exception &e) {
    Logger::logf<LogLevel::ERROR>("Failed to send error response to client fd=%d: %s", fd, e.what());
  } catch (...) {
    Logger::logf<LogLevel::ERROR>("Unknown error sending response to client fd=%d", fd);
  }
}
