#pragma once

#include <cstdint>
#include <unordered_map>
#include <chrono>

namespace Client::Gameplay {

class SkillCooldownTracker {
public:
    struct CooldownEntry {
        uint64_t startTimestampMs{0};
        uint32_t durationMs{0};
    };

    void StartCooldown(uint32_t skillVnum, uint32_t durationMs);
    void StartCooldownWithTimestamp(uint32_t skillVnum, uint32_t durationMs, uint64_t startTimestampMs);
    bool IsOnCooldown(uint32_t skillVnum, uint64_t currentTimestampMs) const;
    uint32_t GetRemainingCooldownMs(uint32_t skillVnum, uint64_t currentTimestampMs) const;
    float GetCooldownProgress(uint32_t skillVnum, uint64_t currentTimestampMs) const;
    void ResetCooldown(uint32_t skillVnum);
    void ResetAll();

    static uint64_t GetCurrentTimeMs() {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
        );
    }

private:
    std::unordered_map<uint32_t, CooldownEntry> m_cooldowns;
};

} // namespace Client::Gameplay
