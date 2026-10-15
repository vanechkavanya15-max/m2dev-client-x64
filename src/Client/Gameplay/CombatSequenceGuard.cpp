#include "CombatSequenceGuard.h"

namespace Client::Gameplay {

Core::Result<uint8_t, Core::CommandError> CombatSequenceGuard::ProcessAttackSequence(
    std::chrono::steady_clock::time_point now,
    std::chrono::milliseconds minInterval) {
    
    if (m_lastAttackTime.time_since_epoch().count() != 0) {
        auto timeSinceLastAttack = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastAttackTime);
        if (timeSinceLastAttack < minInterval) {
            return std::unexpected(Core::CommandError::RateLimited);
        }
    }

    m_lastAttackTime = now;
    m_sequence++;

    // CRC wyliczane jako reszta z dzielenia przez 256 (bajt)
    uint8_t crc = static_cast<uint8_t>(m_sequence % 256);
    return crc;
}

} // namespace Client::Gameplay
