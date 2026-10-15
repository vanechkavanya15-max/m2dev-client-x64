#include "CharacterStatsCache.h"
#include <mutex>
#include <limits>
#include <iostream>

namespace Client::Gameplay {

void CharacterStatsCache::UpdateBonuses(const std::unordered_map<uint8_t, int32_t>& newBonuses) noexcept {
    std::unique_lock lock(m_mutex);
    m_bonusStats.fill(0);
    for (const auto& [bonusType, value] : newBonuses) {
        if (bonusType < MAX_STAT_TYPES) {
            m_bonusStats[bonusType] = value;
        }
    }
    m_isDirty.store(false, std::memory_order_release);
}

StatsResult<int32_t> CharacterStatsCache::GetBonusValue(uint8_t bonusType) const noexcept {
    if (bonusType >= MAX_STAT_TYPES) {
        return std::unexpected(CharacterStatsError::IndexOutOfBounds);
    }
    std::shared_lock lock(m_mutex);
    return m_bonusStats[bonusType];
}

StatsResult<void> CharacterStatsCache::SetBaseStat(uint8_t statType, int32_t value) noexcept {
    if (statType >= MAX_STAT_TYPES) {
        return std::unexpected(CharacterStatsError::IndexOutOfBounds);
    }
    std::unique_lock lock(m_mutex);
    m_baseStats[statType] = value;
    m_isDirty.store(true, std::memory_order_release);
    return {};
}

StatsResult<int32_t> CharacterStatsCache::GetBaseStat(uint8_t statType) const noexcept {
    if (statType >= MAX_STAT_TYPES) {
        return std::unexpected(CharacterStatsError::IndexOutOfBounds);
    }
    std::shared_lock lock(m_mutex);
    return m_baseStats[statType];
}

StatsResult<int32_t> CharacterStatsCache::GetTotalStat(uint8_t statType) const noexcept {
    if (statType >= MAX_STAT_TYPES) {
        return std::unexpected(CharacterStatsError::IndexOutOfBounds);
    }
    
    std::shared_lock lock(m_mutex);
    int32_t base = m_baseStats[statType];
    int32_t bonus = m_bonusStats[statType];
    
    // Check for overflow
    if (bonus > 0 && base > std::numeric_limits<int32_t>::max() - bonus) {
        return std::unexpected(CharacterStatsError::Overflow);
    }
    
    // Check for underflow
    if (bonus < 0 && base < std::numeric_limits<int32_t>::min() - bonus) {
        return std::unexpected(CharacterStatsError::Overflow);
    }
    
    return base + bonus;
}

void CharacterStatsCache::MarkDirty() noexcept {
    m_isDirty.store(true, std::memory_order_release);
}

bool CharacterStatsCache::IsDirty() const noexcept {
    return m_isDirty.load(std::memory_order_acquire);
}

void CharacterStatsCache::Clear() noexcept {
    std::unique_lock lock(m_mutex);
    m_baseStats.fill(0);
    m_bonusStats.fill(0);
    m_isDirty.store(true, std::memory_order_release);
}

} // namespace Client::Gameplay
