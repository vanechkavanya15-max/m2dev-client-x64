#include "SkillCooldownTracker.h"
#include <algorithm>
#include <mutex>

namespace Client::Gameplay {

void SkillCooldownTracker::StartCooldown(uint32_t skillVnum, uint32_t durationMs) {
    StartCooldownWithTimestamp(skillVnum, durationMs, GetCurrentTimeMs());
}

void SkillCooldownTracker::StartCooldownWithTimestamp(uint32_t skillVnum, uint32_t durationMs, uint64_t startTimestampMs) {
    std::unique_lock lock(m_mutex);
    if (durationMs == 0) {
        m_cooldowns.erase(skillVnum);
        return;
    }
    m_cooldowns[skillVnum] = CooldownEntry{startTimestampMs, durationMs};
}

bool SkillCooldownTracker::IsOnCooldown(uint32_t skillVnum, uint64_t currentTimestampMs) const {
    std::shared_lock lock(m_mutex);
    auto it = m_cooldowns.find(skillVnum);
    if (it == m_cooldowns.end()) {
        return false;
    }
    const auto& entry = it->second;
    return (currentTimestampMs >= entry.startTimestampMs && currentTimestampMs < entry.startTimestampMs + entry.durationMs);
}

uint32_t SkillCooldownTracker::GetRemainingCooldownMs(uint32_t skillVnum, uint64_t currentTimestampMs) const {
    std::shared_lock lock(m_mutex);
    auto it = m_cooldowns.find(skillVnum);
    if (it == m_cooldowns.end()) {
        return 0;
    }
    const auto& entry = it->second;
    if (currentTimestampMs < entry.startTimestampMs) {
        return entry.durationMs;
    }
    uint64_t endTime = entry.startTimestampMs + entry.durationMs;
    if (currentTimestampMs >= endTime) {
        return 0;
    }
    return static_cast<uint32_t>(endTime - currentTimestampMs);
}

float SkillCooldownTracker::GetCooldownProgress(uint32_t skillVnum, uint64_t currentTimestampMs) const {
    std::shared_lock lock(m_mutex);
    auto it = m_cooldowns.find(skillVnum);
    if (it == m_cooldowns.end()) {
        return 1.0f; // 100% complete
    }
    const auto& entry = it->second;
    if (entry.durationMs == 0) {
        return 1.0f;
    }
    if (currentTimestampMs >= entry.startTimestampMs + entry.durationMs) {
        return 1.0f;
    }
    if (currentTimestampMs <= entry.startTimestampMs) {
        return 0.0f;
    }
    uint64_t elapsed = currentTimestampMs - entry.startTimestampMs;
    return std::clamp(static_cast<float>(elapsed) / static_cast<float>(entry.durationMs), 0.0f, 1.0f);
}

void SkillCooldownTracker::ResetCooldown(uint32_t skillVnum) {
    std::unique_lock lock(m_mutex);
    m_cooldowns.erase(skillVnum);
}

void SkillCooldownTracker::ResetAll() {
    std::unique_lock lock(m_mutex);
    m_cooldowns.clear();
}

} // namespace Client::Gameplay
