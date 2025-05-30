#include "Poller.hpp"
#include <algorithm>

void Poller::addFd(int fd, short events) {
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = events;
  pfd.revents = 0;
  _poll_fds.push_back(pfd);
}

void Poller::removeFd(int fd) {
  auto it =
      std::find_if(_poll_fds.begin(), _poll_fds.end(),
                   [fd](const struct pollfd &pfd) { return pfd.fd == fd; });
  if (it != _poll_fds.end()) {
    _poll_fds.erase(it);
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
  auto it = std::find_if(_poll_fds.begin(), _poll_fds.end(),
                        [fd](const struct pollfd &pfd) { return pfd.fd == fd; });
  if (it != _poll_fds.end()) {
    it->events = events;
    it->revents = 0;
  }
}

size_t Poller::getFdCount() const {
  return _poll_fds.size();
}

std::vector<struct pollfd> Poller::poll() {
  std::vector<struct pollfd> active_fds;
  if (_poll_fds.empty())
    return active_fds;
  int ret = ::poll(_poll_fds.data(), _poll_fds.size(), _timeout);
  if (ret < 0) {
    if (errno == EINTR) {
      return active_fds;
    } else if (errno == ENOMEM) {
      throw std::runtime_error("poll() failed: Out of memory");
    } else {
      throw std::runtime_error("poll() failed: " + std::string(strerror(errno)) +
                               " (errno: " + std::to_string(errno) + ")");
    }
  }
  if (ret > 0) {
    for (const struct pollfd &pfd : _poll_fds) {
      if (pfd.revents > 0) {
        if (pfd.revents & POLLNVAL) {
          continue;
        }
        active_fds.push_back(pfd);
      }
    }
  }
  return active_fds;
}
bool Poller::hasActivity(const struct pollfd &pfd, short events) const {
  return (pfd.revents & events) == events;
}
bool Poller::empty() const { return _poll_fds.empty(); }
