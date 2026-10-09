#include <mutex>
#include "BuffDurationTracker.h"

namespace Client::Gameplay {

VoidBuffResult BuffDurationTracker::StartBuff(TrackedBuffType buffType, uint32_t durationMs) {
    if (buffType == TrackedBuffType::None) {
        return std::unexpected(BuffTrackerError::InvalidBuffType);
    }
    if (durationMs == 0) {
        return std::unexpected(BuffTrackerError::InvalidDuration);
    }

    std::unique_lock lock(m_mutex);
    
    // Zabezpieczenie przed nadpisywaniem aktywnego buffa bez jawnego odswiezenia
    auto it = m_buffs.find(buffType);
    if (it != m_buffs.end()) {
        uint64_t currentMs = GetCurrentTimeMs();
        if (currentMs < it->second.startTimestampMs + it->second.durationMs) {
            return std::unexpected(BuffTrackerError::AlreadyActive);
        }
    }

    m_buffs[buffType] = BuffEntry{GetCurrentTimeMs(), durationMs};
    return {};
}

VoidBuffResult BuffDurationTracker::StartBuff(TrackedBuffType buffType, std::chrono::milliseconds duration) {
    if (duration.count() <= 0) {
        return std::unexpected(BuffTrackerError::InvalidDuration);
    }
    return StartBuff(buffType, static_cast<uint32_t>(duration.count()));
}

bool BuffDurationTracker::IsBuffActive(TrackedBuffType buffType) const {
    return IsBuffActive(buffType, GetCurrentTimeMs());
}

bool BuffDurationTracker::IsBuffActive(TrackedBuffType buffType, uint64_t currentTimestampMs) const {
    if (buffType == TrackedBuffType::None) {
        return false;
    }

    std::shared_lock lock(m_mutex);
    auto it = m_buffs.find(buffType);
    if (it == m_buffs.end()) {
        return false;
    }
    
    return (currentTimestampMs >= it->second.startTimestampMs && 
            currentTimestampMs < it->second.startTimestampMs + it->second.durationMs);
}

BuffResult<uint32_t> BuffDurationTracker::GetRemainingTimeMs(TrackedBuffType buffType) const {
    return GetRemainingTimeMs(buffType, GetCurrentTimeMs());
}

BuffResult<uint32_t> BuffDurationTracker::GetRemainingTimeMs(TrackedBuffType buffType, uint64_t currentTimestampMs) const {
    if (buffType == TrackedBuffType::None) {
        return std::unexpected(BuffTrackerError::InvalidBuffType);
    }

    std::shared_lock lock(m_mutex);
    auto it = m_buffs.find(buffType);
    if (it == m_buffs.end()) {
        return std::unexpected(BuffTrackerError::NotFound);
    }

    const auto& entry = it->second;
    if (currentTimestampMs < entry.startTimestampMs) {
        // Czasomierz systemowy przesuniety w tyl lub blad w wywolaniu, uznajemy ze caly czas pozostal
        return entry.durationMs;
    }

    uint64_t endTime = entry.startTimestampMs + entry.durationMs;
    if (currentTimestampMs >= endTime) {
        return 0; // Wygaslo
    }

    return static_cast<uint32_t>(endTime - currentTimestampMs);
}

BuffResult<std::chrono::milliseconds> BuffDurationTracker::GetRemainingTime(TrackedBuffType buffType) const {
    auto res = GetRemainingTimeMs(buffType);
    if (!res) {
        return std::unexpected(res.error());
    }
    return std::chrono::milliseconds(res.value());
}

VoidBuffResult BuffDurationTracker::RefreshBuff(TrackedBuffType buffType, uint32_t durationMs) {
    if (buffType == TrackedBuffType::None) {
        return std::unexpected(BuffTrackerError::InvalidBuffType);
    }
    if (durationMs == 0) {
        return std::unexpected(BuffTrackerError::InvalidDuration);
    }

    std::unique_lock lock(m_mutex);
    auto it = m_buffs.find(buffType);
    if (it == m_buffs.end()) {
        return std::unexpected(BuffTrackerError::NotFound);
    }

    // Pozwalamy na odswiezenie tylko, jesli buff jest aktywny (zabezpieczenie domenowe)
    uint64_t currentMs = GetCurrentTimeMs();
    if (currentMs >= it->second.startTimestampMs + it->second.durationMs) {
        return std::unexpected(BuffTrackerError::NotFound);
    }

    it->second.startTimestampMs = currentMs;
    it->second.durationMs = durationMs;
    
    return {};
}

VoidBuffResult BuffDurationTracker::RemoveBuff(TrackedBuffType buffType) {
    if (buffType == TrackedBuffType::None) {
        return std::unexpected(BuffTrackerError::InvalidBuffType);
    }

    std::unique_lock lock(m_mutex);
    auto it = m_buffs.find(buffType);
    if (it == m_buffs.end()) {
        return std::unexpected(BuffTrackerError::NotFound);
    }
    
    // Kasujemy wpis w pelni (szybsze sprawdzanie `find` zamiast trzymania wpisu z `durationMs = 0`)
    m_buffs.erase(it);
    return {};
}

void BuffDurationTracker::ClearAll() {
    std::unique_lock lock(m_mutex);
    m_buffs.clear();
}

} // namespace Client::Gameplay
