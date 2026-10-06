#pragma once

#include <format>
#include <string_view>
#include <iostream>
#include <chrono>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

/**
 * @file LogModern.h
 * @brief Nowoczesny, bezpieczny system logowania C++23 oparty o std::format.
 * 
 * Calkowicie eliminuje podatnosci zwiazane z buforami sprintf / vsprintf oraz
 * umozliwia bezpieczne logowanie dowolnych typow C++23 (w tym StrongType i std::expected).
 */

namespace EterBase {

enum class LogLevel : uint8_t {
    Trace = 0,
    Debug,
    Info,
    Warning,
    Error
};

class ModernLogger {
public:
    template <typename... Args>
    static void Log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        try {
            std::string message = std::format(fmt, std::forward<Args>(args)...);
            WriteLog(level, message);
        } catch (...) {
            // Bezpieczny fallback przy problemie z formatowaniem
            WriteLog(LogLevel::Error, "Log formatting exception occurred");
        }
    }

    template <typename... Args>
    static void Info(std::format_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Info, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Warn(std::format_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Warning, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Error(std::format_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Error, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Debug(std::format_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Warning(std::format_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Warning, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Trace(std::format_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
    }

private:
    static void WriteLog(LogLevel level, std::string_view message) {
        std::string_view prefix = "[INFO]";
        switch (level) {
            case LogLevel::Trace:   prefix = "[TRACE]"; break;
            case LogLevel::Debug:   prefix = "[DEBUG]"; break;
            case LogLevel::Info:    prefix = "[INFO]"; break;
            case LogLevel::Warning: prefix = "[WARN]"; break;
            case LogLevel::Error:   prefix = "[ERROR]"; break;
        }

        std::string fullLine = std::format("{} {}\n", prefix, message);

#ifdef _WIN32
        OutputDebugStringA(fullLine.c_str());
#endif
        if (level == LogLevel::Error || level == LogLevel::Warning) {
            std::cerr << fullLine;
        } else {
            std::cout << fullLine;
        }
    }
};

} // namespace EterBase
