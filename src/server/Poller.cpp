#include "server/Poller.hpp"
#include <algorithm>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include "utils/Logger.hpp"

void Poller::add(int fd, short events) {
    struct pollfd pfd = {fd, events, 0};
    _fds.push_back(pfd);
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
}

void Poller::update(int fd, short events) {
    auto it = std::find_if(
        _fds.begin(),
        _fds.end(),
        [fd](const struct pollfd& pfd) { return pfd.fd == fd; }
    );

    if (it != _fds.end()) {
        it->events = events;
    } else {
        add(fd, events);
    }
}

std::vector<struct pollfd> Poller::poll(int timeout) {
    std::vector<struct pollfd> result;

    if (_fds.empty()) {
        return result;
    }

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
        }
    }

    return result;
}
