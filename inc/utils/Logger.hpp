#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <memory>
#include <cstdio>
#include <vector>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class Logger {
private:
    static LogLevel _currentLevel;
    static std::ofstream _logFile;
    static bool _logToFile;
    static bool _logToConsole;
    static bool _useColors;
    
    // ANSI color codes
    static const std::string COLOR_RESET;
    static const std::string COLOR_DEBUG;   // Cyan
    static const std::string COLOR_INFO;    // Green
    static const std::string COLOR_WARN;    // Yellow
    static const std::string COLOR_ERROR;   // Red
    
    static std::string getCurrentTime();
    static std::string levelToString(LogLevel level);
    static std::string getColorForLevel(LogLevel level);
    static void writeLog(LogLevel level, const std::string& message);

public:
    static void setLevel(LogLevel level);
    static void enableFileLogging(const std::string& filename);
    static void disableFileLogging();
    static void setConsoleLogging(bool enabled);
    static void setColorLogging(bool enabled);
    
#ifdef DEBUG_LOGGING
    static void debug(const std::string& message);
    
    template<typename... Args>
    static void debugf(const std::string& format, Args... args) {
        if (_currentLevel <= LogLevel::DEBUG) {
            debug(formatString(format, args...));
        }
    }
#else
    // No-op functions when debug is disabled
    static void debug(const std::string& /* message */) {}
    
    template<typename... Args>
    static void debugf(const std::string& /* format */, Args... /* args */) {}
#endif
    
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    
    // Convenience methods for formatted logging
    template<typename... Args>
    static void infof(const std::string& format, Args... args) {
        if (_currentLevel <= LogLevel::INFO) {
            info(formatString(format, args...));
        }
    }
    
    template<typename... Args>
    static void warnf(const std::string& format, Args... args) {
        if (_currentLevel <= LogLevel::WARN) {
            warn(formatString(format, args...));
        }
    }
    
    template<typename... Args>
    static void errorf(const std::string& format, Args... args) {
        if (_currentLevel <= LogLevel::ERROR) {
            error(formatString(format, args...));
        }
    }
    
private:
    template<typename... Args>
    static std::string formatString(const std::string& format, Args... args) {
        // Use thread-local static buffer for better performance
        constexpr size_t BUFFER_SIZE = 1024;
        static thread_local std::vector<char> buffer(BUFFER_SIZE);
        
        int size = snprintf(buffer.data(), buffer.size(), format.c_str(), args...);
        
        if (size < 0) {
            return format; // Fallback on formatting error
        }
        
        if (static_cast<size_t>(size) < buffer.size()) {
            // Fast path: message fits in buffer
            return std::string(buffer.data(), size);
        } else {
            // Slow path: message too long, use dynamic allocation
            std::unique_ptr<char[]> dynamicBuf(new char[size + 1]);
            snprintf(dynamicBuf.get(), size + 1, format.c_str(), args...);
            return std::string(dynamicBuf.get(), size);
        }
    }
};
