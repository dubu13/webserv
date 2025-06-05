#pragma once
#include <cstddef>

namespace Constants {
    // HTTP Limits
    constexpr size_t MAX_URI_LENGTH = 2048;
    constexpr size_t MAX_HEADER_SIZE = 8192;
    constexpr size_t MAX_BODY_SIZE = 1048576; // 1MB
    constexpr size_t MAX_CHUNK_SIZE = 65536;  // 64KB
    constexpr size_t MAX_CHUNK_COUNT = 1000;
    constexpr size_t MAX_TOTAL_SIZE = 10 * 1024 * 1024; // 10MB
    constexpr size_t MAX_HEADERS = 100;
    
    // Server Configuration
    constexpr int DEFAULT_PORT = 8080;
    constexpr int CLIENT_TIMEOUT = 30;
    constexpr size_t DEFAULT_CACHE_SIZE = 100;
    constexpr int LISTEN_BACKLOG = 128;
    
    // File System
    constexpr size_t MAX_PATH_LENGTH = 4096;
    constexpr size_t READ_BUFFER_SIZE = 4096;
    
    // HTTP Protocol
    constexpr char CRLF[] = "\r\n";
    constexpr char DOUBLE_CRLF[] = "\r\n\r\n";
    constexpr char LF[] = "\n";
    constexpr char DOUBLE_LF[] = "\n\n";
};
