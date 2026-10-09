#pragma once

#include <chrono>
#include <optional>
#include "Client/Core/StrongTypes.h"
#include "Client/Core/Result.h"

namespace Client::Gameplay {

/**
 * @brief Konfiguracja serwisu do odblokowywania gracza z tekstur.
 */
struct StuckRecoveryConfig {
    std::chrono::milliseconds stuckThresholdDuration{500}; // Czas bez progresu przed cofnieciem
    float minMovementDistance{10.0f};                      // Minimalny dystans by uznac postep
};

/**
 * @brief Kody bledow dla operacji odblokowywania.
 */
enum class StuckRecoveryError : uint8_t {
    NoSafePosition, // Brak dostepnej bezpiecznej pozycji do cofniecia
    NotStuck        // Gracz nie jest zablokowany, brak koniecznosci interwencji
};

/**
 * @brief Komponent odpowiedzialny za wykrywanie zablokowania gracza w teksturach.
 * Zapewnia Zero-conflict C++23 additive architecture.
 */
class StuckRecoveryService {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    constexpr explicit StuckRecoveryService(StuckRecoveryConfig config = {}) noexcept
        : m_config{config} {}

    /**
     * @brief Aktualizuje stan gracza i weryfikuje czy nastapilo zablokowanie.
     * 
     * @param currentPos Obecna pozycja gracza na mapie.
     * @param isMovingKeyPressed Flaga informujaca, czy gracz probuje sie poruszac.
     * @param currentTime Obecny czas pomiaru.
     * @return Bezpieczna pozycja do cofniecia jesli gracz jest zablokowany, w przeciwnym razie blad.
     */
    [[nodiscard]] Client::Core::Result<Client::Core::MapCoords, StuckRecoveryError> 
    Update(const Client::Core::MapCoords& currentPos, bool isMovingKeyPressed, TimePoint currentTime) noexcept;

    /**
     * @brief Resetuje stan serwisu, np. po teleportacji.
     * 
     * @param safePos Nowa zadeklarowana bezpieczna pozycja poczatkowa.
     */
    void Reset(const Client::Core::MapCoords& safePos, TimePoint currentTime) noexcept;

    /**
     * @brief Zwraca ostatnia zarejestrowana bezpieczna pozycje.
     */
    [[nodiscard]] constexpr std::optional<Client::Core::MapCoords> GetSafePosition() const noexcept {
        return m_lastSafePosition;
    }

private:
    StuckRecoveryConfig m_config;
    std::optional<Client::Core::MapCoords> m_lastSafePosition;
    std::optional<Client::Core::MapCoords> m_lastTrackedPosition;
    std::optional<TimePoint> m_lastProgressTime;
};

} // namespace Client::Gameplay
