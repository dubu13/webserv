#include "utils/Logger.hpp"

LogLevel Logger::_currentLevel = LogLevel::INFO;
std::ofstream Logger::_logFile;
bool Logger::_logToFile = false;
bool Logger::_logToConsole = true;
bool Logger::_useColors = true;

const std::string Logger::COLOR_RESET = "\033[0m";
const std::string Logger::COLOR_DEBUG = "\033[36m";
const std::string Logger::COLOR_INFO = "\033[32m";
const std::string Logger::COLOR_WARN = "\033[33m";
const std::string Logger::COLOR_ERROR = "\033[31m";

void Logger::setLevel(LogLevel level) noexcept {
    _currentLevel = level;
}

void Logger::enableFileLogging(std::string_view filename) {
    if (_logFile.is_open()) {
        _logFile.close();
    }
    _logFile.open(std::string(filename), std::ios::app);
    _logToFile = _logFile.is_open();
}

void Logger::disableFileLogging() noexcept {
    if (_logFile.is_open()) {
        _logFile.close();
    }
    _logToFile = false;
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

void Logger::writeLog(LogLevel level, std::string_view message) {
    if (level < _currentLevel) {
        return;
    }

    std::string timestamp = getCurrentTime();
    std::string levelStr = levelToString(level);

    if (_logToConsole) {
        std::string colorCode = getColorForLevel(level);
        std::string resetCode = _useColors ? COLOR_RESET : "";

        std::cout << "[" << timestamp << "] " << colorCode << "[" << levelStr << "]"
                  << resetCode << " " << message << std::endl;
    }

    if (_logToFile && _logFile.is_open()) {
        _logFile << "[" << timestamp << "] [" << levelStr << "] "
                 << message << std::endl;
        _logFile.flush();
    }
}

#ifdef DEBUG_LOGGING
void Logger::debug(std::string_view message) {
    writeLog(LogLevel::DEBUG, message);
}
#endif

void Logger::info(std::string_view message) {
    writeLog(LogLevel::INFO, message);
}

void Logger::warn(std::string_view message) {
    writeLog(LogLevel::WARN, message);
}

void Logger::error(std::string_view message) {
    writeLog(LogLevel::ERROR, message);
}
