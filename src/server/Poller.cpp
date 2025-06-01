#include "Poller.hpp"
#include "utils/Logger.hpp"
#include <algorithm>

void Poller::addFd(int fd, short events) {
  Logger::debugf("Adding fd %d to poller with events 0x%x", fd, events);
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = events;
  pfd.revents = 0;
  _poll_fds.push_back(pfd);
  Logger::debugf("Poller now has %zu file descriptors", _poll_fds.size());
}

void Poller::removeFd(int fd) {
  Logger::debugf("Removing fd %d from poller", fd);
  auto it =
      std::find_if(_poll_fds.begin(), _poll_fds.end(),
                   [fd](const struct pollfd &pfd) { return pfd.fd == fd; });
  if (it != _poll_fds.end()) {
    _poll_fds.erase(it);
    Logger::debugf("Fd %d removed. Poller now has %zu file descriptors", fd, _poll_fds.size());
  } else {
    Logger::warnf("Attempted to remove fd %d but it was not found in poller", fd);
  }
}

void Poller::updateFd(int fd, short events) {
  auto it = std::find_if(_poll_fds.begin(), _poll_fds.end(),
                        [fd](const struct pollfd &pfd) { return pfd.fd == fd; });
  if (it != _poll_fds.end()) {
    it->events = events;
    it->revents = 0;
  } else {
    addFd(fd, events);
  }
}

void Poller::setFdEvents(int fd, short events) {
  Logger::debugf("Setting events 0x%x for fd %d", events, fd);
  auto it = std::find_if(_poll_fds.begin(), _poll_fds.end(),
                        [fd](const struct pollfd &pfd) { return pfd.fd == fd; });
  if (it != _poll_fds.end()) {
    short oldEvents = it->events;
    it->events = events;
    it->revents = 0;
    Logger::debugf("Updated fd %d events from 0x%x to 0x%x", fd, oldEvents, events);
  } else {
    Logger::warnf("Attempted to set events for fd %d but it was not found in poller", fd);
  }
}

size_t Poller::getFdCount() const {
  return _poll_fds.size();
}

std::vector<struct pollfd> Poller::poll() {
  std::vector<struct pollfd> active_fds;
  if (_poll_fds.empty()) {
    Logger::debug("Poll called with no file descriptors");
    return active_fds;
  }
  
  Logger::debugf("Calling poll() on %zu file descriptors with timeout %d ms", _poll_fds.size(), _timeout);
  int ret = ::poll(_poll_fds.data(), _poll_fds.size(), _timeout);
  
  if (ret < 0) {
    if (errno == EINTR) {
      Logger::debug("Poll interrupted by signal");
      return active_fds;
    } else if (errno == ENOMEM) {
      Logger::error("Poll failed: Out of memory");
      throw std::runtime_error("poll() failed: Out of memory");
    } else {
      Logger::errorf("Poll failed: %s (errno: %d)", strerror(errno), errno);
      throw std::runtime_error("poll() failed: " + std::string(strerror(errno)) +
                               " (errno: " + std::to_string(errno) + ")");
    }
  }
  
  if (ret == 0) {
    Logger::debug("Poll timeout - no activity");
    return active_fds;
  }
  
  Logger::debugf("Poll returned %d active file descriptors", ret);
  for (const struct pollfd &pfd : _poll_fds) {
    if (pfd.revents > 0) {
      if (pfd.revents & POLLNVAL) {
        Logger::warnf("Invalid fd %d detected in poll results", pfd.fd);
        continue;
      }
      Logger::debugf("Active fd: %d, events: 0x%x, revents: 0x%x", pfd.fd, pfd.events, pfd.revents);
      active_fds.push_back(pfd);
    }
  }
  
  return active_fds;
}
bool Poller::hasActivity(const struct pollfd &pfd, short events) const {
  return (pfd.revents & events) == events;
}
bool Poller::empty() const { return _poll_fds.empty(); }
