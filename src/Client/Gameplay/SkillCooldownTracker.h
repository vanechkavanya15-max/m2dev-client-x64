#pragma once

#include <cstdint>
#include <unordered_map>
#include <chrono>
#include <shared_mutex>

namespace Client::Gameplay {

class SkillCooldownTracker {
public:
    struct CooldownEntry {
        uint64_t startTimestampMs{0};
        uint32_t durationMs{0};
    };

    void StartCooldown(uint32_t skillVnum, uint32_t durationMs);
    void StartCooldown(uint32_t skillVnum, std::chrono::milliseconds duration);
    void StartCooldownWithTimestamp(uint32_t skillVnum, uint32_t durationMs, uint64_t startTimestampMs);
    void StartCooldownWithTimePoint(uint32_t skillVnum, std::chrono::milliseconds duration, std::chrono::steady_clock::time_point startTime);

    bool IsOnCooldown(uint32_t skillVnum) const;
    bool IsOnCooldown(uint32_t skillVnum, uint64_t currentTimestampMs) const;
    bool IsOnCooldown(uint32_t skillVnum, std::chrono::steady_clock::time_point currentTime) const;

    uint32_t GetRemainingCooldownMs(uint32_t skillVnum) const;
    uint32_t GetRemainingCooldownMs(uint32_t skillVnum, uint64_t currentTimestampMs) const;
    std::chrono::milliseconds GetRemainingCooldown(uint32_t skillVnum) const;
    std::chrono::milliseconds GetRemainingCooldown(uint32_t skillVnum, std::chrono::steady_clock::time_point currentTime) const;

    float GetCooldownProgress(uint32_t skillVnum) const;
    float GetCooldownProgress(uint32_t skillVnum, uint64_t currentTimestampMs) const;
    float GetCooldownProgress(uint32_t skillVnum, std::chrono::steady_clock::time_point currentTime) const;

    void ResetCooldown(uint32_t skillVnum);
    void ResetAll();

    static uint64_t GetCurrentTimeMs() noexcept {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
        );
    }

    static uint64_t TimePointToMs(std::chrono::steady_clock::time_point tp) noexcept {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                tp.time_since_epoch()
            ).count()
        );
    }

private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<uint32_t, CooldownEntry> m_cooldowns;
};

} // namespace Client::Gameplay
