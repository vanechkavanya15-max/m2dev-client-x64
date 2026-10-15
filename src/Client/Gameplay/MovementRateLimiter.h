#pragma once

#include <chrono>

namespace Client::Gameplay {

/**
 * @class MovementRateLimiter
 * @brief Zero-conflict C++23 rate limiter for character movement synchronization.
 * 
 * Emulates the server's movement broadcast interval (e.g., 250ms). It buffers
 * micro-movements and determines when a new CG_CHARACTER_MOVE packet should be
 * generated or skipped to avoid flooding the server.
 */
class MovementRateLimiter {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::milliseconds;

    /**
     * @brief Constructs a new MovementRateLimiter.
     * @param interval The minimum time between movement packets (default 250ms).
     */
    explicit MovementRateLimiter(Duration interval = std::chrono::milliseconds(250));

    /**
     * @brief Decides whether a movement packet should be sent at the given time.
     * 
     * @param currentTime The current time to check against.
     * @param force If true, bypasses the interval check and forces an update.
     * @return true if the packet should be sent, false otherwise.
     */
    [[nodiscard]] bool ShouldSendMovementPacket(TimePoint currentTime, bool force = false);

    /**
     * @brief Resets the rate limiter state.
     */
    void Reset() noexcept;

    /**
     * @brief Gets the configured interval.
     * @return The current minimum duration between packets.
     */
    [[nodiscard]] Duration GetInterval() const noexcept { return m_interval; }
    
    /**
     * @brief Sets a new interval.
     * @param interval The new minimum duration between packets.
     */
    void SetInterval(Duration interval) noexcept { m_interval = interval; }

private:
    Duration m_interval;
    TimePoint m_lastSendTime;
    bool m_hasSentFirstPacket;
};

} // namespace Client::Gameplay
