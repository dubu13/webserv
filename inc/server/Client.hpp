#pragma once

#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdexcept>

class Client {
private:
    int _socketFd;
    struct sockaddr_in _address;
    std::string _incomingData;
    std::string _outgoingData;
    bool _keepAlive;
    time_t _lastActivity;
    
public:
    // Constructors and destructor
    Client(int socketFd, const struct sockaddr_in& address);
    ~Client();
    
    // Disable copy (C++98 style)
    Client(const Client& other);
    Client& operator=(const Client& other);
    
    // Getters
    int getSocketFd() const;
    const struct sockaddr_in& getAddress() const;
    std::string getIpAddress() const;
    int getPort() const;
    
    // Connection management
    bool isKeepAlive() const;
    void setKeepAlive(bool keepAlive);
    time_t getLastActivity() const;
    void updateActivity();
    bool hasTimedOut(time_t timeout) const;
    
    // Data management
    const std::string& getIncomingData() const;
    const std::string& getOutgoingData() const;
    void appendIncomingData(const std::string& data);
    void clearIncomingData();
    void setOutgoingData(const std::string& data);
    void clearOutgoingData();
    bool hasCompleteRequest() const;
    
    // I/O operations
    ssize_t readData();
    ssize_t writeData();
    bool hasDataToWrite() const;
    
    // Exceptions
    class ClientException : public std::runtime_error {
    public:
        explicit ClientException(const std::string& message);
    };
};