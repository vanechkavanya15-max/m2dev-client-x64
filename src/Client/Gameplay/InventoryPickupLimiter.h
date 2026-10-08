#pragma once

#include <cstdint>
#include <expected>
#include "../Core/DomainCommands.h"
#include "../Core/StrongTypes.h"
#include "../../EterBase/Result.h"

namespace Client::Gameplay {

/**
 * @brief Ogranicznik podnoszenia przedmiotów. Weryfikuje zasięg i czas między podniesieniami.
 */
class InventoryPickupLimiter {
public:
    static constexpr float MAX_PICKUP_RANGE = 300.0f;
    static constexpr uint32_t PICKUP_COOLDOWN_MS = 100;

    /**
     * @brief Sprawdza czy można podnieść przedmiot.
     * @param playerCoords Pozycja gracza
     * @param itemCoords Pozycja przedmiotu
     * @param currentTimestamp Aktualny czas w milisekundach
     * @return Result z błędem w przypadku przekroczenia limitu (OutOfRange, RateLimited)
     */
    [[nodiscard]] EterBase::Result<void, Client::Core::CommandError> CanPickup(
        const Client::Core::MapCoords& playerCoords, 
        const Client::Core::MapCoords& itemCoords, 
        uint32_t currentTimestamp);

private:
    uint32_t m_lastPickupTime = 0;
};

} // namespace Client::Gameplay
