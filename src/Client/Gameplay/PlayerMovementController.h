#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <chrono>
#include "EterBase/Result.h"
#include "Client/Core/StrongTypes.h"
#include "Client/Core/DomainCommands.h"
#include "Client/Gameplay/MovementTerrainCollider.h"
#include "Client/Gameplay/MovementRateLimiter.h"

namespace Client::Gameplay {

/**
 * @class PlayerMovementController
 * @brief Odpowiada za koordynacje ruchu gracza, laczac walidacje kolizji terenu 
 *        (MovementTerrainCollider) i kontrole czestotliwosci pakietow (MovementRateLimiter).
 */
class PlayerMovementController {
public:
    /**
     * @brief Konstruktor kontrolera ruchu.
     * @param provider Wskaznik na dostawce atrybutow terenu dla kolidera.
     * @param limitInterval Odstep czasu dla rate limitera (np. 250ms).
     */
    explicit PlayerMovementController(const ITerrainAttributeProvider* provider,
                                      MovementRateLimiter::Duration limitInterval = std::chrono::milliseconds(250));

    ~PlayerMovementController() = default;

    PlayerMovementController(const PlayerMovementController&) = delete;
    PlayerMovementController& operator=(const PlayerMovementController&) = delete;
    PlayerMovementController(PlayerMovementController&&) = default;
    PlayerMovementController& operator=(PlayerMovementController&&) = default;

    /**
     * @brief Aktualizuje pozycje gracza. Funkcja weryfikuje ruch przy pomocy collidera 
     *        (przeszkody i slizganie) i decyduje (rate limiter) czy nalezy wygenerowac 
     *        pakiet do serwera (CG_MOVE).
     *
     * @param nextCoords Cel, do ktorego gracz chce sie przemiescic.
     * @param currentRotation Obecna rotacja gracza w stopniach.
     * @param moveType Typ ruchu (np. 0 = chodzenie, 1 = bieganie).
     * @param currentClientTime Aktualny czas klienta przekazywany do komendy.
     * @param currentTime Aktualny punkt czasu lokalnego zegara do weryfikacji czestotliwosci (rate limit).
     * @param forceSend Flaga by wymusic wyslanie pakietu (np. start/stop ruchu).
     * 
     * @return Zwraca Result. Przy bledzie zawiera CommandError (np. MovementBlocked). 
     *         Przy sukcesie moze zawierac zakodowany pakiet lub std::nullopt (zignorowany dla optymalizacji sieci).
     */
    [[nodiscard]] EterBase::Result<std::optional<std::vector<uint8_t>>, Client::Core::CommandError>
    UpdateMovement(const Client::Core::MapCoords& nextCoords,
                   float currentRotation,
                   uint8_t moveType,
                   uint32_t currentClientTime,
                   MovementRateLimiter::TimePoint currentTime,
                   bool forceSend = false);

    /**
     * @brief Wymusza ustawienie pozycji klienta (np. warp, serwerowy respawn).
     */
    void Teleport(const Client::Core::MapCoords& coords) noexcept;

    /**
     * @brief Zwraca biezaca pozycje gracza utrzymywana przez kontroler.
     */
    [[nodiscard]] Client::Core::MapCoords GetCurrentPosition() const noexcept { return m_currentPosition; }

private:
    MovementTerrainCollider m_collider;
    MovementRateLimiter m_rateLimiter;
    Client::Core::MapCoords m_currentPosition;
};

} // namespace Client::Gameplay
