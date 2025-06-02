#include "server/Poller.hpp"
#include <algorithm>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include "utils/Logger.hpp"

void Poller::add(int fd, short events) {
    struct pollfd pfd = {fd, events, 0};
    _fds.push_back(pfd);
    updateLastActivity(fd);
}

void Poller::remove(int fd) {
    _fds.erase(
        std::remove_if(
            _fds.begin(),
            _fds.end(),
            [fd](const struct pollfd& pfd) { return pfd.fd == fd; }
        ),
        _fds.end()
    );
    _lastActivity.erase(fd);
}

void Poller::update(int fd, short events) {
    auto it = std::find_if(
        _fds.begin(),
        _fds.end(),
        [fd](const struct pollfd& pfd) { return pfd.fd == fd; }
    );

    if (it != _fds.end()) {
        it->events = events;
        updateLastActivity(fd);
    } else {
        add(fd, events);
    }
}

std::vector<struct pollfd> Poller::poll(int timeout) {
    std::vector<struct pollfd> result;

    if (_fds.empty()) {
        return result;
    }

    removeTimedOutConnections();

    int ret = ::poll(_fds.data(), _fds.size(), timeout);

    if (ret < 0) {
        Logger::errorf("Poll error: %s", strerror(errno));
        return result;
    }

    if (ret == 0) {
        return result;
    }

    for (const auto& pfd : _fds) {
        if (pfd.revents != 0) {
            result.push_back(pfd);
            updateLastActivity(pfd.fd);
        }
    }

    return result;
}

void Poller::updateLastActivity(int fd) {
    _lastActivity[fd] = std::chrono::steady_clock::now();
}

bool Poller::hasTimedOut(int fd) const {
    auto it = _lastActivity.find(fd);
    if (it == _lastActivity.end()) {
        return true;
    }

    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
    return duration >= DEFAULT_TIMEOUT;
}

void Poller::removeTimedOutConnections() {
    std::vector<int> timedOutFds;

    for (const auto& [fd, _] : _lastActivity) {
        if (hasTimedOut(fd)) {
            timedOutFds.push_back(fd);
            Logger::info("Connection timed out: " + std::to_string(fd));
        }
    }

    for (int fd : timedOutFds) {
        remove(fd);
        close(fd);
    }
}
