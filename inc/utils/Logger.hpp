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

    template<typename... Args>
    static void debugf(std::string_view format, Args&&... args) {
        if (_currentLevel <= LogLevel::DEBUG) {
            debug(formatString(format, std::forward<Args>(args)...));
        }
    }
#else

    static void debug(std::string_view) {}

    template<typename... Args>
    static void debugf(std::string_view, Args&&...) {}
#endif

    static void info(std::string_view message);
    static void warn(std::string_view message);
    static void error(std::string_view message);

    template<typename... Args>
    static void infof(std::string_view format, Args&&... args) {
        if (_currentLevel <= LogLevel::INFO) {
            info(formatString(format, std::forward<Args>(args)...));
        }
    }

    template<typename... Args>
    static void warnf(std::string_view format, Args&&... args) {
        if (_currentLevel <= LogLevel::WARN) {
            warn(formatString(format, std::forward<Args>(args)...));
        }
    }

    template<typename... Args>
    static void errorf(std::string_view format, Args&&... args) {
        if (_currentLevel <= LogLevel::ERROR) {
            error(formatString(format, std::forward<Args>(args)...));
        }
    }

private:
    template<typename... Args>
    static std::string formatString(std::string_view format, Args&&... args) {
        if constexpr (sizeof...(args) == 0) {
            return std::string(format);
        } else {
            std::ostringstream ss;
            formatStringImpl(ss, format, std::forward<Args>(args)...);
            return ss.str();
        }
    }

    template<typename T, typename... Args>
    static void formatStringImpl(std::ostringstream& ss, std::string_view format, T&& value, Args&&... args) {
        size_t pos = format.find('%');
        if (pos == std::string::npos) {
            ss << format;
            return;
        }

        ss << format.substr(0, pos);

        size_t nextPos = pos + 1;
        while (nextPos < format.size() &&
               (format[nextPos] == '-' || format[nextPos] == '+' ||
                format[nextPos] == '0' || format[nextPos] == '#' ||
                format[nextPos] == ' ' || std::isdigit(format[nextPos]) ||
                format[nextPos] == '.' || format[nextPos] == '*' ||
                format[nextPos] == 'h' || format[nextPos] == 'l' ||
                format[nextPos] == 'j' || format[nextPos] == 'z' ||
                format[nextPos] == 't' || format[nextPos] == 'L')) {
            nextPos++;
        }

        if (nextPos < format.size()) {

            nextPos++;

            ss << value;

            if (nextPos < format.size()) {
                formatStringImpl(ss, format.substr(nextPos), std::forward<Args>(args)...);
            }
        }
    }

    static void formatStringImpl(std::ostringstream& ss, std::string_view format) {
        ss << format;
    }
};
