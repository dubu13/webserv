#include "Server.hpp"
#include "ClientHandler.hpp"
#include "utils/Logger.hpp"
#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <memory>

extern bool g_running;

Server::Server(const ServerBlock& config) 
    : _server_fd(-1), _config(config), _running(false) {
    if (getrlimit(RLIMIT_NOFILE, &_rlim) == -1) {
        throw std::runtime_error("Failed to get file descriptor limit");
    }
    Logger::infof("System allows %ld file descriptors", _rlim.rlim_cur);
    
    // Extract port from listen directives for display
    int port = 8080; // default
    if (!config.listenDirectives.empty()) {
        port = config.listenDirectives[0].second;
    }
    Logger::infof("Server will listen on %s:%d", config.host.c_str(), port);
    
    // Create the ClientHandler with configuration  
    _clientHandler = std::make_unique<ClientHandler>(*this, config.root, 60);  // TODO: make timeout configurable
}

Server::~Server() {
  if (_server_fd >= 0) {
    close(_server_fd);
  }
  // std::unique_ptr automatically cleans up _clientHandler
}

void Server::setupSocket() {
  Logger::debug("Starting socket setup...");
  _server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (_server_fd == -1) {
    Logger::errorf("Failed to create socket: %s", strerror(errno));
    throw std::runtime_error("Failed to create socket");
  }
  Logger::debugf("Socket created successfully (fd: %d)", _server_fd);
  
  int opt = 1;
  if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    Logger::errorf("Failed to set SO_REUSEADDR: %s", strerror(errno));
    close(_server_fd);
    throw std::runtime_error("Failed to set socket options");
  }
  Logger::debug("SO_REUSEADDR option set successfully");
  
  _address.sin_family = AF_INET;
  // Extract port from listen directives
  int port = 8080; // default
  if (!_config.listenDirectives.empty()) {
    port = _config.listenDirectives[0].second;
  }
  _address.sin_port = htons(port);
  Logger::debugf("Setting up socket for %s:%d", _config.host.c_str(), port);
  
  if (inet_pton(AF_INET, _config.host.c_str(), &_address.sin_addr) <= 0) {
    Logger::errorf("Failed to convert host address '%s': %s", _config.host.c_str(), strerror(errno));
    close(_server_fd);
    throw std::runtime_error("Failed to convert host address");
  }
  Logger::debugf("Host address '%s' converted successfully", _config.host.c_str());
  
  if (bind(_server_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0) {
    Logger::errorf("Failed to bind socket to %s:%d: %s", _config.host.c_str(), port, strerror(errno));
    close(_server_fd);
    throw std::runtime_error("Failed to bind socket");
  }
  Logger::debugf("Socket bound successfully to %s:%d", _config.host.c_str(), port);
  
  if (listen(_server_fd, SOMAXCONN) < 0) {
    Logger::errorf("Failed to listen on socket: %s", strerror(errno));
    close(_server_fd);
    throw std::runtime_error("Failed to listen on socket");
  }
  Logger::debugf("Socket listening with backlog %d", SOMAXCONN);
  
  setNonBlocking(_server_fd);
  Logger::info("Socket setup complete - non-blocking mode enabled");
}

void Server::setNonBlocking(int fd) {
  Logger::debugf("Setting non-blocking mode for fd: %d", fd);
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    Logger::errorf("Failed to get file descriptor flags for fd %d: %s", fd, strerror(errno));
    throw std::runtime_error("Failed to set non-blocking mode");
  }
  Logger::debugf("Current flags for fd %d: 0x%x", fd, flags);
  
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    Logger::errorf("Failed to set O_NONBLOCK flag for fd %d: %s", fd, strerror(errno));
    throw std::runtime_error("Failed to set non-blocking mode");
  }
  Logger::debugf("Successfully set non-blocking mode for fd: %d", fd);
}

void Server::run() {
  _running = true;
  Logger::debug("Server run() starting - setting up socket and poller");
  setupSocket();
  _poller.addFd(_server_fd, POLLIN);
  Logger::debugf("Added server socket (fd: %d) to poller with POLLIN events", _server_fd);
  
  int port = 8080; // default
  if (!_config.listenDirectives.empty()) {
    port = _config.listenDirectives[0].second;
  }
  Logger::infof("Server listening on %s:%d", _config.host.c_str(), port);
  Logger::debug("Entering main event loop");
  
  while (g_running && _running) {
    try {
      Logger::debugf("Polling %zu file descriptors...", _poller.getFdCount());
      auto active_fds = _poller.poll();
      Logger::debugf("Poll returned %zu active file descriptors", active_fds.size());
      
      for (const auto &pfd : active_fds) {
        Logger::debugf("Processing fd: %d, events: 0x%x", pfd.fd, pfd.revents);
        
        if (isServerSocket(pfd.fd)) {
          if (_poller.hasActivity(pfd, POLLIN)) {
            Logger::debug("Server socket has incoming connection");
            acceptConnection();
          }
        } else {
          Logger::debugf("Client socket activity on fd: %d", pfd.fd);
          handleClientEvent(pfd.fd, pfd.revents);
        }
      }
      _clientHandler->checkTimeouts();
    } catch (const std::exception &e) {
      Logger::errorf("Server error in main loop: %s", e.what());
    }
  }
  Logger::info("Server main loop exited");
}

void Server::acceptConnection() {
  Logger::debug("Attempting to accept new connection");
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);
  int client_fd =
      accept(_server_fd, (struct sockaddr *)&client_addr, &addr_len);
  if (client_fd < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      Logger::errorf("Accept failed: %s (errno: %d)", strerror(errno), errno);
    } else {
      Logger::debug("Accept would block - no incoming connections");
    }
    return;
  }
  
  Logger::debugf("New connection accepted (fd: %d)", client_fd);
  
  // Check file descriptor limits (subject requirement: never crash, even when out of resources)
  if (_clientHandler->getClientCount() >= _rlim.rlim_cur - 10) {
    Logger::warnf("Rejecting connection - too many clients (%zu/%ld file descriptors)", 
                  _clientHandler->getClientCount(), _rlim.rlim_cur);
    close(client_fd);
    return;
  }
  
  try {
    setNonBlocking(client_fd);
    _clientHandler->addClient(client_fd, client_addr);
    Logger::debugf("Client successfully added and configured (fd: %d)", client_fd);
  } catch (const std::exception &e) {
    Logger::errorf("Failed to configure new client (fd: %d): %s", client_fd, e.what());
    close(client_fd);
  }
}

void Server::handleClientEvent(int fd, short events) {
  Logger::debugf("Handling client event for fd: %d, events: 0x%x", fd, events);
  
  if (events & POLLERR) {
    Logger::debugf("POLLERR detected on fd: %d", fd);
    _clientHandler->removeClient(fd);
  } else if (events & POLLHUP) {
    Logger::debugf("POLLHUP detected on fd: %d - client disconnected", fd);
    _clientHandler->removeClient(fd);
  } else if (events & POLLIN) {
    Logger::debugf("POLLIN event on fd: %d - data available for reading", fd);
    _clientHandler->handleRead(fd);
  } else if (events & POLLOUT) {
    Logger::debugf("POLLOUT event on fd: %d - ready for writing", fd);
    _clientHandler->handleWrite(fd);
  } else {
    Logger::debugf("Unhandled event 0x%x on fd: %d", events, fd);
  }
}

void Server::stop() {
  _running = false;
  if (_server_fd >= 0) {
    close(_server_fd);
    _server_fd = -1;
  }
}
