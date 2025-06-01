#pragma once
#include "utils/FileDescriptor.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <ctime>

namespace server {

class Client {
public:
    Client(int socketFd, const struct sockaddr_in& addr);
    Client(Client&& other) noexcept;
    Client& operator=(Client&& other) noexcept;
    
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    
    int getSocketFd() const { return _socket.get(); }
    bool isValid() const noexcept { return _socket.isValid(); }
    std::string getIpAddress() const;
    int getPort() const noexcept { return ntohs(_address.sin_port); }
    void updateActivity() noexcept { _lastActivity = time(nullptr); }
    bool hasTimedOut(time_t timeout) const noexcept;
    bool hasDataToWrite() const { return !_outgoingData.empty(); }

    std::string& getIncomingData() { return _incomingData; }
    const std::string& getIncomingData() const { return _incomingData; }
    std::string& getOutgoingData() { return _outgoingData; }
    const std::string& getOutgoingData() const { return _outgoingData; }
    
    void setKeepAlive(bool keepAlive) { _keepAlive = keepAlive; }
    bool isKeepAlive() const { return _keepAlive; }
    void clearIncomingData() { _incomingData.clear(); }
    void clearOutgoingData() { _outgoingData.clear(); }
    time_t getLastActivity() const { return _lastActivity; }
    void swap(Client& other) noexcept;

private:
    utils::FileDescriptor _socket;
    struct sockaddr_in _address;
    std::string _incomingData;
    std::string _outgoingData;
    bool _keepAlive;
    time_t _lastActivity;
};

} // namespace server