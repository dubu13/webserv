#include "utils/Logger.hpp"
#include <cstdio>
#include <memory>

// Static member definitions
LogLevel Logger::_currentLevel = LogLevel::INFO;
std::ofstream Logger::_logFile;
bool Logger::_logToFile = false;
bool Logger::_logToConsole = true;
bool Logger::_useColors = true;

// ANSI color codes
const std::string Logger::COLOR_RESET = "\033[0m";
const std::string Logger::COLOR_DEBUG = "\033[36m";   // Cyan
const std::string Logger::COLOR_INFO = "\033[32m";    // Green  
const std::string Logger::COLOR_WARN = "\033[33m";    // Yellow
const std::string Logger::COLOR_ERROR = "\033[31m";   // Red

void Logger::setLevel(LogLevel level) {
    _currentLevel = level;
}

void Logger::enableFileLogging(const std::string& filename) {
    if (_logFile.is_open()) {
        _logFile.close();
    }
    _logFile.open(filename, std::ios::app);
    _logToFile = _logFile.is_open();
}

void Logger::disableFileLogging() {
    if (_logFile.is_open()) {
        _logFile.close();
    }
    _logToFile = false;
}

void Logger::setConsoleLogging(bool enabled) {
    _logToConsole = enabled;
}

void Logger::setColorLogging(bool enabled) {
    _useColors = enabled;
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKN ";
    }
}

std::string Logger::getColorForLevel(LogLevel level) {
    if (!_useColors) {
        return "";
    }
    
    switch (level) {
        case LogLevel::DEBUG: return COLOR_DEBUG;
        case LogLevel::INFO:  return COLOR_INFO;
        case LogLevel::WARN:  return COLOR_WARN;
        case LogLevel::ERROR: return COLOR_ERROR;
        default: return "";
    }
}

void Logger::writeLog(LogLevel level, const std::string& message) {
    if (level < _currentLevel) {
        return;
    }
    
    std::string timestamp = getCurrentTime();
    std::string levelStr = levelToString(level);
    
    // For console output with colors
    if (_logToConsole) {
        std::string colorCode = getColorForLevel(level);
        std::string resetCode = _useColors ? COLOR_RESET : "";
        std::string coloredLogLine = "[" + timestamp + "] " + colorCode + "[" + levelStr + "]" + resetCode + " " + message;
        
        if (level >= LogLevel::ERROR) {
            std::cerr << coloredLogLine << std::endl;
        } else {
            std::cout << coloredLogLine << std::endl;
        }
    }
    
    // For file output without colors
    if (_logToFile && _logFile.is_open()) {
        std::string plainLogLine = "[" + timestamp + "] [" + levelStr + "] " + message;
        _logFile << plainLogLine << std::endl;
        _logFile.flush();
    }
}

void Logger::debug(const std::string& message) {
    writeLog(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    writeLog(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    writeLog(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    writeLog(LogLevel::ERROR, message);
}
