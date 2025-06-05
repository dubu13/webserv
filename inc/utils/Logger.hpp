#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>
#include <memory>

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

    static const std::string COLOR_RESET;
    static const std::string COLOR_DEBUG;
    static const std::string COLOR_INFO;
    static const std::string COLOR_WARN;
    static const std::string COLOR_ERROR;

    static std::string getCurrentTime();
    static std::string levelToString(LogLevel level);
    static std::string getColorForLevel(LogLevel level);
    static void writeLog(LogLevel level, std::string_view message);

public:
    static void setLevel(LogLevel level) noexcept;
    static void enableFileLogging(std::string_view filename);
    static void disableFileLogging() noexcept;

#ifdef DEBUG_LOGGING
    static void debug(std::string_view message);
#else

    static void debug(std::string_view) {}
#endif

    static void info(std::string_view message);
    static void warn(std::string_view message);
    static void error(std::string_view message);

    template<LogLevel Level, typename... Args>
    static void logf(std::string_view format, Args&&... args) {
        if (_currentLevel <= Level) {
            writeLog(Level, simpleFormat(format, std::forward<Args>(args)...));
        }
    }

private:
    template<typename T>
    static std::string toString(T&& value) {
        std::ostringstream ss;
        ss << value;
        return ss.str();
    }
    
    template<typename... Args>
    static std::string simpleFormat(std::string_view format, Args&&... args) {
        if constexpr (sizeof...(args) == 0) {
            return std::string(format);
        } else {
            std::ostringstream result;
            std::vector<std::string> values = {toString(std::forward<Args>(args))...};
            
            size_t argIndex = 0;
            for (char c : format) {
                if (c == '%' && argIndex < values.size()) {
                    result << values[argIndex++];
                } else if (c != '%') {
                    result << c;
                }
            }
            return result.str();
        }
    }
};
