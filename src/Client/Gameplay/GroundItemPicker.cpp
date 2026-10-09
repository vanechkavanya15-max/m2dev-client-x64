#include "GroundItemPicker.h"
#include <limits>
#include <optional>

namespace Client::Gameplay {

EterBase::Result<PickResult, PickerError> GroundItemPicker::PickNearestItem(
    std::span<const GroundItem> items, 
    const Client::Core::MapCoords& playerCoords,
    const std::string& playerName)
{
    if (items.empty()) {
        return EterBase::MakeError(PickerError::NoItemsNearby);
    }

    float minDistance = std::numeric_limits<float>::max();
    std::optional<Client::Core::EntityVid> bestVid = std::nullopt;
    bool anyItemInRange = false;

    for (const auto& item : items) {
        float distance = playerCoords.Distance(item.coords);

        if (distance > MAX_PICKUP_RANGE) {
            continue;
        }
        
        anyItemInRange = true;

        // Sprawdzenie wlasnosci. Pusta etykieta oznacza ze kazdy moze podniesc.
        if (!item.ownershipLabel.empty() && item.ownershipLabel != playerName) {
            continue;
        }

        if (distance < minDistance) {
            minDistance = distance;
            bestVid = item.vid;
        }
    }
    
    if (!anyItemInRange) {
        return EterBase::MakeError(PickerError::AllItemsOutOfRange);
    }

    if (!bestVid.has_value()) {
        return EterBase::MakeError(PickerError::NoOwnership);
    }

    return PickResult{ *bestVid, minDistance };
}

} // namespace Client::Gameplay
