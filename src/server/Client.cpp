#include "server/Client.hpp"
#include "utils/Logger.hpp"
#include <cstring>

namespace server {

Client::Client(int socketFd, const struct sockaddr_in& addr)
    : _socket(socketFd), _address(addr), _keepAlive(false), _lastActivity(time(nullptr)) {
    Logger::debugf("Client created for fd: %d", socketFd);
}

Client::Client(Client&& other) noexcept
    : _socket(std::move(other._socket))
    , _address(other._address)
    , _incomingData(std::move(other._incomingData))
    , _outgoingData(std::move(other._outgoingData))
    , _keepAlive(other._keepAlive)
    , _lastActivity(other._lastActivity) {
    Logger::debugf("Client moved from fd: %d to fd: %d", other._socket.get(), _socket.get());
}

Client& Client::operator=(Client&& other) noexcept {
    if (this != &other) {
        Client temp(std::move(other));
        swap(temp);
        Logger::debugf("Client move-assigned using swap idiom to fd: %d", _socket.get());
    }
    return *this;
}

void Client::swap(Client& other) noexcept {
    std::swap(_socket, other._socket);
    std::swap(_address, other._address);
    std::swap(_incomingData, other._incomingData);
    std::swap(_outgoingData, other._outgoingData);
    std::swap(_keepAlive, other._keepAlive);
    std::swap(_lastActivity, other._lastActivity);
}

std::string Client::getIpAddress() const {
    char ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &_address.sin_addr, ip, INET_ADDRSTRLEN) == nullptr) {
        Logger::warn("Failed to convert client IP address to string");
        return "unknown";
    }
    return std::string(ip);
}

bool Client::hasTimedOut(time_t timeout) const noexcept {
    return (time(nullptr) - _lastActivity) > timeout;
}

} // namespace server
