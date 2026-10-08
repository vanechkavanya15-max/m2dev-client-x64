#pragma once

#include <cstdint>
#include <shared_mutex>
#include <unordered_map>
#include <array>

namespace Client::Gameplay {

class PlayerStatsDomain {
public:
    PlayerStatsDomain() = default;
    ~PlayerStatsDomain() = default;

    int64_t GetPoint(uint32_t pointType) const;
    void SetPoint(uint32_t pointType, int64_t value);
    
    uint32_t GetLevel() const;
    uint32_t GetHP() const;
    uint32_t GetMaxHP() const;
    uint32_t GetSP() const;
    uint32_t GetMaxSP() const;
    int64_t GetGold() const;
    uint64_t GetExp() const;
    
    void SetStatusPoint(uint32_t stIndex, uint16_t value);
    uint16_t GetStatusPoint(uint32_t stIndex) const;

private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<uint32_t, int64_t> m_points;
    std::array<uint16_t, 255> m_statusPoints{}; // Or appropriate size, though map could be safer if index is unbounded
};

} // namespace Client::Gameplay
