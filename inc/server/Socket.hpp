#pragma once

#include <netinet/in.h> //for struct sockaddr_in
#include <sys/socket.h> //for socket functions
#include <unistd.h> //for close()
#include <fcntl.h> //for fcntl()
#include <stdexcept> //for std::runtime_error
#include <iostream>
#include <string.h>

class Socket {
    private:
        int _server_fd;
        struct sockaddr_in _address;
        int _port;
        std::string _host;
    public:
        Socket(int port = 8080, const std::string& host = "0.0.0.0");
        ~Socket();

        void createSocket();
        void setSocketOptions();
        void bindSocket();
        static void setNonBlocking(int fd);
        void startListening();
        int getFd() const;
};
