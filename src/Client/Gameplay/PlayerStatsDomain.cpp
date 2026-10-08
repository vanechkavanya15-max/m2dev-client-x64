#include "PlayerStatsDomain.h"
#include <algorithm>
#include <mutex>

namespace Client::Gameplay {

// Map points conceptually according to EPointTypes or arbitrary enums.
// Assuming EPointTypes from Packet.h for typical values like:
// 1 = POINT_LEVEL, 3 = POINT_EXP, 5 = POINT_HP, 6 = POINT_MAX_HP, 7 = POINT_SP, 8 = POINT_MAX_SP, 11 = POINT_GOLD
constexpr uint32_t POINT_LEVEL = 1;
constexpr uint32_t POINT_EXP = 3;
constexpr uint32_t POINT_HP = 5;
constexpr uint32_t POINT_MAX_HP = 6;
constexpr uint32_t POINT_SP = 7;
constexpr uint32_t POINT_MAX_SP = 8;
constexpr uint32_t POINT_GOLD = 11;

int64_t PlayerStatsDomain::GetPoint(uint32_t pointType) const {
    std::shared_lock lock(m_mutex);
    auto it = m_points.find(pointType);
    return it != m_points.end() ? it->second : 0;
}

void PlayerStatsDomain::SetPoint(uint32_t pointType, int64_t value) {
    std::unique_lock lock(m_mutex);
    
    // Non-negative clamping for HP and SP
    if (pointType == POINT_HP || pointType == POINT_SP) {
        value = std::max<int64_t>(0, value);
    }
    
    m_points[pointType] = value;
}

uint32_t PlayerStatsDomain::GetLevel() const {
    return static_cast<uint32_t>(GetPoint(POINT_LEVEL));
}

uint32_t PlayerStatsDomain::GetHP() const {
    return static_cast<uint32_t>(GetPoint(POINT_HP));
}

uint32_t PlayerStatsDomain::GetMaxHP() const {
    return static_cast<uint32_t>(GetPoint(POINT_MAX_HP));
}

uint32_t PlayerStatsDomain::GetSP() const {
    return static_cast<uint32_t>(GetPoint(POINT_SP));
}

uint32_t PlayerStatsDomain::GetMaxSP() const {
    return static_cast<uint32_t>(GetPoint(POINT_MAX_SP));
}

int64_t PlayerStatsDomain::GetGold() const {
    return GetPoint(POINT_GOLD);
}

uint64_t PlayerStatsDomain::GetExp() const {
    return static_cast<uint64_t>(GetPoint(POINT_EXP));
}

void PlayerStatsDomain::SetStatusPoint(uint32_t stIndex, uint16_t value) {
    std::unique_lock lock(m_mutex);
    if (stIndex < m_statusPoints.size()) {
        m_statusPoints[stIndex] = value;
    }
}

uint16_t PlayerStatsDomain::GetStatusPoint(uint32_t stIndex) const {
    std::shared_lock lock(m_mutex);
    if (stIndex < m_statusPoints.size()) {
        return m_statusPoints[stIndex];
    }
    return 0;
}

} // namespace Client::Gameplay
