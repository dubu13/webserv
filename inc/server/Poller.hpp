#pragma once
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <stdexcept>
#include <vector>
class Poller {
private:
  std::vector<struct pollfd> _poll_fds;
  static const int _timeout = 1000;

public:
  void addFd(int fd, short events);
  void removeFd(int fd);
  void updateFd(int fd, short events);
  std::vector<struct pollfd> poll();
  bool hasActivity(const struct pollfd &pfd, short events) const;
  bool empty() const;
};
