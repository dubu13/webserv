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
        // int _port;
    public:
        Socket();
        ~Socket();

        void createSocket();
        void setSocketOptions();
        void bindSocket();
        static void setNonBlocking(int fd);
        void startListening();
        int getFd() const;
};
