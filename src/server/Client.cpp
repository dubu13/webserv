#include "Client.hpp"
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <errno.h>
Client::Client(int socketFd, const struct sockaddr_in& address)
    : _socketFd(socketFd),
      _address(address),
      _keepAlive(false)
{
    _lastActivity = time(NULL);
}
Client::~Client() {
}
Client::Client(const Client& other)
    : _socketFd(other._socketFd),
      _address(other._address),
      _incomingData(other._incomingData),
      _outgoingData(other._outgoingData),
      _keepAlive(other._keepAlive),
      _lastActivity(other._lastActivity) {}
Client& Client::operator=(const Client& other) {
    if (this != &other) {
        _socketFd = other._socketFd;
        _address = other._address;
        _incomingData = other._incomingData;
        _outgoingData = other._outgoingData;
        _keepAlive = other._keepAlive;
        _lastActivity = other._lastActivity;
    }
    return *this;
}
int Client::getSocketFd() const {
    return _socketFd;
}
const struct sockaddr_in& Client::getAddress() const {
    return _address;
}
std::string Client::getIpAddress() const {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &_address.sin_addr, ip, INET_ADDRSTRLEN);
    return std::string(ip);
}
int Client::getPort() const {
    return ntohs(_address.sin_port);
}
bool Client::isKeepAlive() const {
    return _keepAlive;
}
void Client::setKeepAlive(bool keepAlive) {
    _keepAlive = keepAlive;
}
time_t Client::getLastActivity() const {
    return _lastActivity;
}
void Client::updateActivity() {
    _lastActivity = time(NULL);
}
bool Client::hasTimedOut(time_t timeout) const {
    return (time(NULL) - _lastActivity) > timeout;
}
const std::string& Client::getIncomingData() const {
    return _incomingData;
}
const std::string& Client::getOutgoingData() const {
    return _outgoingData;
}
void Client::appendIncomingData(const std::string& data) {
    _incomingData.append(data);
    updateActivity();
}
void Client::clearIncomingData() {
    _incomingData.clear();
}
void Client::setOutgoingData(const std::string& data) {
    _outgoingData = data;
    updateActivity();
}
void Client::clearOutgoingData() {
    _outgoingData.clear();
}
bool Client::hasCompleteRequest() const {
    return _incomingData.find("\r\n\r\n") != std::string::npos;
}
ssize_t Client::readData() {
    char buffer[4096] = {0};
    ssize_t bytes_read = read(_socketFd, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        appendIncomingData(buffer);
    }
    return bytes_read;
}
ssize_t Client::writeData() {
    if (_outgoingData.empty())
        return 0;
    ssize_t bytes_written = write(_socketFd, _outgoingData.c_str(), _outgoingData.size());
    if (bytes_written > 0) {
        _outgoingData.erase(0, bytes_written);
    }
    return bytes_written;
}
bool Client::hasDataToWrite() const {
    return !_outgoingData.empty();
}