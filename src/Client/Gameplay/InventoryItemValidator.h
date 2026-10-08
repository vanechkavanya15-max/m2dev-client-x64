#pragma once

#include <cstdint>
#include "../../EterBase/Result.h"
#include "../../Client/Core/DomainCommands.h"

class CItemData;

namespace Client::Gameplay {
    class InventoryItemValidator {
    public:
        static EterBase::Result<bool, Client::Core::CommandError> CanEquipItem(
            const CItemData* itemData,
            uint8_t playerRace,
            uint8_t playerLevel,
            uint16_t targetCell);
    };
}
