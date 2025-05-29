#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <time.h>
class Client {
private:
  int _socketFd;
  struct sockaddr_in _address;
  std::string _incomingData;
  std::string _outgoingData;
  bool _keepAlive;
  time_t _lastActivity;

public:
  Client(int socketFd, const struct sockaddr_in &address);
  ~Client();
  Client(const Client &other);
  Client &operator=(const Client &other);
  int getSocketFd() const;
  const struct sockaddr_in &getAddress() const;
  std::string getIpAddress() const;
  int getPort() const;
  bool isKeepAlive() const;
  void setKeepAlive(bool keepAlive);
  time_t getLastActivity() const;
  void updateActivity();
  bool hasTimedOut(time_t timeout) const;
  const std::string &getIncomingData() const;
  const std::string &getOutgoingData() const;
  void appendIncomingData(const std::string &data);
  void clearIncomingData();
  void setOutgoingData(const std::string &data);
  void clearOutgoingData();
  bool hasCompleteRequest() const;
  ssize_t readData();
  ssize_t writeData();
  bool hasDataToWrite() const;
};
