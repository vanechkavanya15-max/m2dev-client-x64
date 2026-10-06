#pragma once

#include <chrono>
#include <cstdint>
#include <expected>
#include <string_view>
#include <format>
#include "LogModern.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace EterBase {

/**
 * @brief High resolution clock error enumeration.
 */
enum class ClockError : uint8_t {
    None = 0,
    NotSupported,
    QueryFailed
};

/**
 * @brief Converts ClockError to a string representation for logging and debugging.
 * @param err The clock error.
 * @return String view of the error.
 */
[[nodiscard]] constexpr std::string_view ToString(ClockError err) noexcept {
    switch (err) {
        case ClockError::None: return "None";
        case ClockError::NotSupported: return "NotSupported (High resolution timer not available)";
        case ClockError::QueryFailed: return "QueryFailed (Failed to read performance counter)";
    }
    return "UnknownClockError";
}

} // namespace EterBase

// Enable std::format for ClockError
template <>
struct std::formatter<EterBase::ClockError> : std::formatter<std::string_view> {
    auto format(EterBase::ClockError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(EterBase::ToString(err), ctx);
    }
};

namespace EterBase {

/**
 * @class HighResMicroClock
 * @brief A high-resolution clock designed to provide microsecond precision using QueryPerformanceCounter on Windows.
 * 
 * Provides modern C++23 error handling via std::expected and uses safe logging mechanisms (EterBase::ModernLogger).
 */
class HighResMicroClock {
public:
    /**
     * @brief Initializes the clock by querying the performance frequency.
     * @return Result containing true on success or a ClockError on failure.
     */
    [[nodiscard]] static std::expected<void, ClockError> Initialize() noexcept {
#ifdef _WIN32
        LARGE_INTEGER freq;
        if (!QueryPerformanceFrequency(&freq)) {
            ModernLogger::Error("HighResMicroClock::Initialize failed: QueryPerformanceFrequency returned false.");
            return std::unexpected(ClockError::NotSupported);
        }
        
        if (freq.QuadPart == 0) {
            ModernLogger::Error("HighResMicroClock::Initialize failed: Performance frequency is zero.");
            return std::unexpected(ClockError::NotSupported);
        }
        
        frequency = freq.QuadPart;
        
        LARGE_INTEGER counter;
        if (!QueryPerformanceCounter(&counter)) {
            ModernLogger::Error("HighResMicroClock::Initialize failed: QueryPerformanceCounter returned false.");
            return std::unexpected(ClockError::QueryFailed);
        }
        
        baseTime = counter.QuadPart;
        ModernLogger::Info("HighResMicroClock::Initialize successful. Frequency: {}", frequency);
        return {};
#else
        // Fallback for non-Windows systems (e.g., during syntax checking on Linux)
        ModernLogger::Warn("HighResMicroClock::Initialize called on non-Windows system.");
        return std::unexpected(ClockError::NotSupported);
#endif
    }

    /**
     * @brief Retrieves the time elapsed since initialization in microseconds.
     * @return Result containing the elapsed time in microseconds, or a ClockError on failure.
     */
    [[nodiscard]] static std::expected<uint64_t, ClockError> GetElapsedMicroseconds() noexcept {
#ifdef _WIN32
        if (frequency == 0) {
            ModernLogger::Error("HighResMicroClock::GetElapsedMicroseconds called before successful Initialize().");
            return std::unexpected(ClockError::NotSupported);
        }

        LARGE_INTEGER counter;
        if (!QueryPerformanceCounter(&counter)) {
            ModernLogger::Error("HighResMicroClock::GetElapsedMicroseconds failed: QueryPerformanceCounter returned false.");
            return std::unexpected(ClockError::QueryFailed);
        }
        
        uint64_t elapsedCounts = counter.QuadPart - baseTime;
        
        // Calculate securely to prevent overflow in long-running processes
        uint64_t seconds = elapsedCounts / frequency;
        uint64_t remainderCounts = elapsedCounts % frequency;
        uint64_t elapsedMicroseconds = (seconds * 1'000'000ULL) + ((remainderCounts * 1'000'000ULL) / frequency);
        
        return elapsedMicroseconds;
#else
        ModernLogger::Warn("HighResMicroClock::GetElapsedMicroseconds called on non-Windows system.");
        return std::unexpected(ClockError::NotSupported);
#endif
    }

private:
    static inline uint64_t frequency = 0;
    static inline uint64_t baseTime = 0;
};

} // namespace EterBase
