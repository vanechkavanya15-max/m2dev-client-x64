#pragma once

#include <vector>
#include <cstdint>
#include "../Core/DomainCommands.h"
#include "../Core/Result.h"
#include "../../EterBase/StrongTypes.h"

namespace Client::Gameplay {

class InventoryGridManager {
public:
    InventoryGridManager(uint16_t width, uint16_t height, uint16_t pages);
    ~InventoryGridManager() = default;

    Client::Core::Result<void, Client::Core::CommandError> CanPlaceItem(EterBase::ItemSlot slot, uint8_t itemHeight) const;
    Client::Core::Result<EterBase::ItemSlot, Client::Core::CommandError> FindEmptySlot(uint8_t itemHeight) const;
    Client::Core::Result<void, Client::Core::CommandError> OccupySlot(EterBase::ItemSlot slot, uint8_t itemHeight);
    Client::Core::Result<void, Client::Core::CommandError> FreeSlot(EterBase::ItemSlot slot, uint8_t itemHeight);

private:
    uint16_t m_width;
    uint16_t m_height;
    uint16_t m_pages;
    uint16_t m_pageSize;
    uint16_t m_totalSize;
    std::vector<bool> m_grid;
};

} // namespace Client::Gameplay
