#pragma once

#include <cstdint>
#include <chrono>
#include <unordered_map>
#include <algorithm>

/**
 * @brief Manages cooldown timers for skills, items, and potions without coupling to the GUI.
 * 
 * This class tracks the remaining time for various actions and determines whether 
 * an entity (such as a skill or a potion) is ready for immediate use.
 */
class CooldownTracker
{
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::milliseconds;

    /**
     * @brief Structure holding cooldown information for a specific entity.
     */
    struct CooldownInfo
    {
        TimePoint startTime;
        Duration maxDuration;
    };

    /**
     * @brief Default constructor for CooldownTracker.
     */
    CooldownTracker() = default;

    /**
     * @brief Default destructor for CooldownTracker.
     */
    ~CooldownTracker() = default;

    // Prevent copying to avoid accidental desynchronization of cooldowns.
    CooldownTracker(const CooldownTracker&) = delete;
    CooldownTracker& operator=(const CooldownTracker&) = delete;

    // Allow move semantics.
    CooldownTracker(CooldownTracker&&) = default;
    CooldownTracker& operator=(CooldownTracker&&) = default;

    /**
     * @brief Registers or updates a cooldown for a specific skill or item.
     * 
     * @param id The unique identifier for the skill or item.
     * @param durationMs The cooldown duration in milliseconds.
     */
    inline void RegisterCooldown(uint32_t id, uint32_t durationMs)
    {
        if (durationMs == 0)
        {
            ClearCooldown(id);
            return;
        }

        cooldowns[id] = CooldownInfo{
            Clock::now(),
            Duration(durationMs)
        };
    }

    /**
     * @brief Checks if the skill or item is ready for immediate use.
     * 
     * @param id The unique identifier for the skill or item.
     * @return true if there is no active cooldown (ready to use), false otherwise.
     */
    inline bool IsReady(uint32_t id) const
    {
        auto it = cooldowns.find(id);
        if (it == cooldowns.end())
        {
            return true;
        }

        auto now = Clock::now();
        auto elapsedTime = std::chrono::duration_cast<Duration>(now - it->second.startTime);

        return elapsedTime >= it->second.maxDuration;
    }

    /**
     * @brief Retrieves the remaining cooldown duration for a specific skill or item.
     * 
     * @param id The unique identifier for the skill or item.
     * @return The remaining cooldown duration in milliseconds. Returns 0 if ready.
     */
    inline uint32_t GetRemainingCooldown(uint32_t id) const
    {
        auto it = cooldowns.find(id);
        if (it == cooldowns.end())
        {
            return 0;
        }

        auto now = Clock::now();
        auto elapsedTime = std::chrono::duration_cast<Duration>(now - it->second.startTime);

        if (elapsedTime >= it->second.maxDuration)
        {
            return 0;
        }

        auto remainingTime = it->second.maxDuration - elapsedTime;
        return static_cast<uint32_t>(remainingTime.count());
    }

    /**
     * @brief Retrieves the maximum cooldown duration registered for the identifier.
     * 
     * @param id The unique identifier for the skill or item.
     * @return The maximum duration in milliseconds, or 0 if not found.
     */
    inline uint32_t GetMaxCooldown(uint32_t id) const
    {
        auto it = cooldowns.find(id);
        if (it == cooldowns.end())
        {
            return 0;
        }

        return static_cast<uint32_t>(it->second.maxDuration.count());
    }

    /**
     * @brief Clears the cooldown for a specific identifier, making it ready.
     * 
     * @param id The unique identifier for the skill or item.
     */
    inline void ClearCooldown(uint32_t id)
    {
        cooldowns.erase(id);
    }

    /**
     * @brief Clears all tracked cooldowns, resetting all states.
     */
    inline void ClearAll()
    {
        cooldowns.clear();
    }

private:
    std::unordered_map<uint32_t, CooldownInfo> cooldowns;
};
