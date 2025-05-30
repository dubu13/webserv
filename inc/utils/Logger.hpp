#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <memory>
#include <cstdio>

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
    
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    
    // Convenience methods for formatted logging
    template<typename... Args>
    static void debugf(const std::string& format, Args... args) {
        if (_currentLevel <= LogLevel::DEBUG) {
            debug(formatString(format, args...));
        }
    }
    
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
        size_t size = snprintf(nullptr, 0, format.c_str(), args...) + 1;
        std::unique_ptr<char[]> buf(new char[size]);
        snprintf(buf.get(), size, format.c_str(), args...);
        return std::string(buf.get(), buf.get() + size - 1);
    }
};
