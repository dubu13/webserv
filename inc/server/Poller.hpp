#pragma once

#include <vector>
#include <poll.h>
#include <map>
#include <chrono>

class Poller {
public:
    static const int DEFAULT_TIMEOUT = 30000;

    Poller() = default;
    ~Poller() = default;

    Poller(const Poller&) = delete;
    Poller& operator=(const Poller&) = delete;

    Poller(Poller&&) = default;
    Poller& operator=(Poller&&) = default;

    void add(int fd, short events);
    void remove(int fd);
    void update(int fd, short events);
    std::vector<struct pollfd> poll(int timeout = DEFAULT_TIMEOUT);

    void updateLastActivity(int fd);
    bool hasTimedOut(int fd) const;
    void removeTimedOutConnections();

private:
    std::vector<struct pollfd> _fds;
    std::map<int, std::chrono::steady_clock::time_point> _lastActivity;
};
