#pragma once

#include <cstdint>
#include <optional>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "IInventoryService.h"

namespace UserInterface::Services
{
    /**
     * @brief Interfejs mikro-serwisu specjalistycznych ekwipunkow (Pas, Smocza Alchemia, Kostiumy).
     */
    class ISpecialInventoryService
    {
    public:
        virtual ~ISpecialInventoryService() = default;

        virtual EterBase::PacketResult<void> SetBeltItem(EterBase::ItemSlot slot, const InventoryItemView& item) = 0;
        virtual EterBase::PacketResult<void> RemoveBeltItem(EterBase::ItemSlot slot) = 0;
        virtual std::optional<InventoryItemView> GetBeltItem(EterBase::ItemSlot slot) const = 0;

        virtual EterBase::PacketResult<void> SetDragonSoulItem(uint8_t deck, EterBase::ItemSlot slot, const InventoryItemView& item) = 0;
        virtual EterBase::PacketResult<void> RemoveDragonSoulItem(uint8_t deck, EterBase::ItemSlot slot) = 0;
        virtual void SetDragonSoulDeckActive(uint8_t deck, bool active) = 0;
        virtual bool IsDragonSoulDeckActive(uint8_t deck) const = 0;

        virtual void Clear() = 0;
    };
}
