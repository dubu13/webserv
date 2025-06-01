#pragma once
#include "utils/FileDescriptor.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <ctime>

namespace server {

/**
 * @brief RAII-managed client connection
 * 
 * Encapsulates a client socket connection with automatic resource management.
 * Uses FileDescriptor for RAII socket handling and manages client state/data.
 * 
 * Features:
 * - Automatic socket cleanup via RAII
 * - Connection state management
 * - Buffer management for HTTP data
 * - Activity tracking for timeouts
 * - Move semantics for efficient transfers
 */
class Client {
public:
    /**
     * @brief Construct a new Client from an accepted socket
     * @param socketFd Raw socket file descriptor (ownership transferred)
     * @param addr Client's socket address information
     */
    Client(int socketFd, const struct sockaddr_in& addr);
    
    /**
     * @brief Move constructor
     */
    Client(Client&& other) noexcept;
    
    /**
     * @brief Move assignment operator
     */
    Client& operator=(Client&& other) noexcept;
    
    // Disable copy operations
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    
    /**
     * @brief Get the raw socket file descriptor
     * @return int Socket file descriptor or -1 if invalid
     */
    int getSocketFd() const { return _socket.get(); }
    
    /**
     * @brief Check if socket is valid
     * @return bool True if socket is valid and connected
     */
    bool isValid() const { return _socket.isValid(); }
    
    /**
     * @brief Get client IP address as string
     * @return std::string IP address in dotted decimal notation
     */
    std::string getIpAddress() const;
    
    /**
     * @brief Get client port number
     * @return int Port number
     */
    int getPort() const { return ntohs(_address.sin_port); }
    
    /**
     * @brief Update last activity timestamp
     */
    void updateActivity() { _lastActivity = time(nullptr); }
    
    /**
     * @brief Check if client has timed out
     * @param timeout Timeout duration in seconds
     * @return bool True if client has exceeded timeout
     */
    bool hasTimedOut(time_t timeout) const;
    
    /**
     * @brief Check if client has data waiting to be written
     * @return bool True if outgoing buffer has data
     */
    bool hasDataToWrite() const { return !_outgoingData.empty(); }
    
    /**
     * @brief Get reference to incoming data buffer
     * @return std::string& Incoming HTTP data buffer
     */
    std::string& getIncomingData() { return _incomingData; }
    const std::string& getIncomingData() const { return _incomingData; }
    
    /**
     * @brief Get reference to outgoing data buffer
     * @return std::string& Outgoing HTTP response buffer
     */
    std::string& getOutgoingData() { return _outgoingData; }
    const std::string& getOutgoingData() const { return _outgoingData; }
    
    /**
     * @brief Set keep-alive connection flag
     * @param keepAlive True for persistent connections
     */
    void setKeepAlive(bool keepAlive) { _keepAlive = keepAlive; }
    
    /**
     * @brief Check if connection should be kept alive
     * @return bool True for persistent connections
     */
    bool isKeepAlive() const { return _keepAlive; }
    
    /**
     * @brief Clear incoming data buffer
     */
    void clearIncomingData() { _incomingData.clear(); }
    
    /**
     * @brief Clear outgoing data buffer  
     */
    void clearOutgoingData() { _outgoingData.clear(); }
    
    /**
     * @brief Get last activity timestamp
     * @return time_t Timestamp of last activity
     */
    time_t getLastActivity() const { return _lastActivity; }

private:
    utils::FileDescriptor _socket;      ///< RAII-managed socket file descriptor
    struct sockaddr_in _address;       ///< Client socket address
    std::string _incomingData;          ///< Buffer for incoming HTTP request data
    std::string _outgoingData;          ///< Buffer for outgoing HTTP response data  
    bool _keepAlive;                    ///< Keep-alive connection flag
    time_t _lastActivity;               ///< Timestamp of last client activity
};

} // namespace server