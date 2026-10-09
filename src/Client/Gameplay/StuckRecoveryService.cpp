#include "StuckRecoveryService.h"

namespace Client::Gameplay {

Client::Core::Result<Client::Core::MapCoords, StuckRecoveryError> 
StuckRecoveryService::Update(const Client::Core::MapCoords& currentPos, bool isMovingKeyPressed, TimePoint currentTime) noexcept {
    if (!isMovingKeyPressed) {
        // Jesli nie ma intencji ruchu, traktujemy obecna pozycje jako bezpieczna i resetujemy licznik.
        m_lastSafePosition = currentPos;
        m_lastProgressTime = currentTime;
        m_lastTrackedPosition = currentPos;
        return std::unexpected(StuckRecoveryError::NotStuck);
    }

    if (!m_lastProgressTime.has_value() || !m_lastTrackedPosition.has_value()) {
        m_lastProgressTime = currentTime;
        m_lastTrackedPosition = currentPos;
        m_lastSafePosition = currentPos;
        return std::unexpected(StuckRecoveryError::NotStuck);
    }

    const float distanceMoved = currentPos.Distance(*m_lastTrackedPosition);

    if (distanceMoved >= m_config.minMovementDistance) {
        // Nastapil postep, aktualizujemy stan jako bezpieczny.
        m_lastSafePosition = currentPos;
        m_lastProgressTime = currentTime;
        m_lastTrackedPosition = currentPos;
        return std::unexpected(StuckRecoveryError::NotStuck);
    }

    // Brak wystarczajacego postepu. Sprawdzamy jak dlugo to trwa.
    const auto timeSinceProgress = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - *m_lastProgressTime);

    if (timeSinceProgress >= m_config.stuckThresholdDuration) {
        // Przekroczono prog zablokowania.
        if (m_lastSafePosition.has_value()) {
            // Zwracamy punkt do cofniecia, resetujemy czas by nie cofac wielokrotnie w tej samej klatce.
            m_lastProgressTime = currentTime; 
            return *m_lastSafePosition;
        } else {
            return std::unexpected(StuckRecoveryError::NoSafePosition);
        }
    }

    // Jeszcze nie minal prog czasu
    return std::unexpected(StuckRecoveryError::NotStuck);
}

void StuckRecoveryService::Reset(const Client::Core::MapCoords& safePos, TimePoint currentTime) noexcept {
    m_lastSafePosition = safePos;
    m_lastTrackedPosition = safePos;
    m_lastProgressTime = currentTime;
}

} // namespace Client::Gameplay
