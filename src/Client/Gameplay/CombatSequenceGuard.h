#pragma once

#include <chrono>
#include <cstdint>
#include "../Core/DomainCommands.h"
#include "../Core/Result.h"

namespace Client::Gameplay {

class CombatSequenceGuard {
public:
    CombatSequenceGuard() = default;
    ~CombatSequenceGuard() = default;

    /// @brief Checks anti-speedattack limits and generates the next attack sequence CRC.
    /// @param now The current time point.
    /// @param minInterval The minimum allowed time between attacks (anti-speedattack guard).
    /// @return Result containing the calculated bCRC on success, or CommandError::RateLimited if attacks are too fast.
    [[nodiscard]] Core::Result<uint8_t, Core::CommandError> ProcessAttackSequence(
        std::chrono::steady_clock::time_point now,
        std::chrono::milliseconds minInterval);

private:
    uint32_t m_dwSeq{0};
    std::chrono::steady_clock::time_point m_lastAttackTime{};
};

} // namespace Client::Gameplay
