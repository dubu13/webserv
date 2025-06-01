#pragma once
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <stdexcept>
#include <vector>
#include <chrono>

class Poller {
private:
  std::vector<struct pollfd> _poll_fds;
  static const inline int _timeout = 1000;  // Add 'inline' here
public:
  void addFd(int fd, short events);
  void removeFd(int fd);
  void updateFd(int fd, short events);
  std::vector<struct pollfd> poll();
  bool hasActivity(const struct pollfd &pfd, short events) const;
  bool empty() const;
  void setFdEvents(int fd, short events);
  size_t getFdCount() const;
};
