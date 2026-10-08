#include "InventoryPickupLimiter.h"

namespace Client::Gameplay {

EterBase::Result<void, Client::Core::CommandError> InventoryPickupLimiter::CanPickup(
    const Client::Core::MapCoords& playerCoords, 
    const Client::Core::MapCoords& itemCoords, 
    uint32_t currentTimestamp) 
{
    // Special case: Initial pickup (m_lastPickupTime is 0), prevent first-frame false positive rate limit
    if (m_lastPickupTime != 0 && (currentTimestamp - m_lastPickupTime < PICKUP_COOLDOWN_MS)) {
        return std::unexpected(Client::Core::CommandError::RateLimited);
    }

    // Check distance (slower check due to math)
    if (playerCoords.Distance(itemCoords) > MAX_PICKUP_RANGE) {
        return std::unexpected(Client::Core::CommandError::OutOfRange);
    }

    // Success, update last pickup time
    m_lastPickupTime = currentTimestamp;
    
    return {};
}

} // namespace Client::Gameplay
