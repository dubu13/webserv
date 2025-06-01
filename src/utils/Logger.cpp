#include "utils/Logger.hpp"

// Static member definitions
LogLevel Logger::m_currentLevel = LogLevel::INFO;
std::ofstream Logger::m_logFile;
bool Logger::m_logToFile = false;
bool Logger::m_logToConsole = true;
bool Logger::m_useColors = true;

// ANSI color codes
const std::string Logger::COLOR_RESET = "\033[0m";
const std::string Logger::COLOR_DEBUG = "\033[36m";   // Cyan
const std::string Logger::COLOR_INFO = "\033[32m";    // Green  
const std::string Logger::COLOR_WARN = "\033[33m";    // Yellow
const std::string Logger::COLOR_ERROR = "\033[31m";   // Red

void Logger::setLevel(LogLevel level) noexcept {
    m_currentLevel = level;
}

void Logger::enableFileLogging(std::string_view filename) {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    m_logFile.open(std::string(filename), std::ios::app);
    m_logToFile = m_logFile.is_open();
}

void Logger::disableFileLogging() noexcept {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    m_logToFile = false;
}

void Logger::setConsoleLogging(bool enabled) noexcept {
    m_logToConsole = enabled;
}

void Logger::setColorLogging(bool enabled) noexcept {
    m_useColors = enabled;
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
    if (!m_useColors) {
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
    if (level < m_currentLevel) {
        return;
    }
    
    std::string timestamp = getCurrentTime();
    std::string levelStr = levelToString(level);
    
    // For console output with colors
    if (m_logToConsole) {
        std::string colorCode = getColorForLevel(level);
        std::string resetCode = m_useColors ? COLOR_RESET : "";
        
        std::cout << "[" << timestamp << "] " << colorCode << "[" << levelStr << "]" 
                  << resetCode << " " << message << std::endl;
    }
    
    // For file output without colors
    if (m_logToFile && m_logFile.is_open()) {
        m_logFile << "[" << timestamp << "] [" << levelStr << "] " 
                 << message << std::endl;
        m_logFile.flush();
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
