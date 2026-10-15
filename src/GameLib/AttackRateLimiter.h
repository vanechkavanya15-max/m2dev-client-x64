#pragma once

#include <cstdint>
#include <deque>
#include <stdexcept>

/**
 * @class AttackRateLimiter
 * @brief Manages the rate of attacks to prevent excessive packet sending (Anti-Kick protection).
 * 
 * This class implements a sliding window algorithm to ensure that no more than
 * a specified number of attacks occur within a given time window. It updates purely
 * the in-memory state and is decoupled from any GUI elements.
 */
class AttackRateLimiter {
public:
    /**
     * @brief Constructs an AttackRateLimiter.
     * @param limit The maximum number of attacks allowed within the time window.
     * @param window_ms The duration of the time window in milliseconds.
     * @throws std::invalid_argument if limit or window_ms is zero.
     */
    AttackRateLimiter(uint32_t limit, uint32_t window_ms)
        : limit(limit), window_ms(window_ms) {
        if (limit == 0 || window_ms == 0) {
            throw std::invalid_argument("Limit and window_ms must be greater than zero.");
        }
    }

    /**
     * @brief Checks if an attack is allowed to be sent based on the current rate.
     * 
     * If the attack is allowed, its timestamp is recorded. Old timestamps
     * outside the sliding window are discarded.
     * 
     * @param current_time_ms The current time in milliseconds.
     * @return true if the attack is allowed, false if the rate limit is exceeded.
     */
    [[nodiscard]] bool CheckLimit(uint32_t current_time_ms) {
        // Handle potential timer wrap-around or backwards time jump
        if (!attack_times.empty() && current_time_ms < attack_times.back()) {
            attack_times.clear();
        }

        // Remove old attacks that are outside the current window
        while (!attack_times.empty() && (current_time_ms - attack_times.front() > window_ms)) {
            attack_times.pop_front();
        }

        // Check if we have exceeded the limit
        if (attack_times.size() >= limit) {
            return false;
        }

        // Record the new attack
        attack_times.push_back(current_time_ms);
        return true;
    }

    /**
     * @brief Resets the attack rate limiter, clearing all recorded attacks.
     */
    void Reset() {
        attack_times.clear();
    }

private:
    uint32_t limit;
    uint32_t window_ms;
    std::deque<uint32_t> attack_times;
};
