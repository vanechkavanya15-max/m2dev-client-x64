#pragma once

#include <cstdint>
#include <optional>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::Services
{
    /**
     * @brief Struktura opisujaca pojedynczy przedmiot w inwentarzu gracza.
     */
    struct InventoryItemView
    {
        EterBase::ItemSlot slot{0};
        EterBase::ItemVnum vnum{0};
        uint32_t count{0};
        int32_t sockets[6]{0};
        int16_t attrTypes[7]{0};
        int16_t attrValues[7]{0};
        bool isLocked{false};
    };

    /**
     * @brief Interfejs mikro-serwisu obslugi ekwipunku gracza.
     */
    class IInventoryService
    {
    public:
        virtual ~IInventoryService() = default;

        virtual EterBase::PacketResult<void> SetItem(EterBase::ItemSlot slot, const InventoryItemView& item) = 0;
        virtual EterBase::PacketResult<void> RemoveItem(EterBase::ItemSlot slot) = 0;
        virtual std::optional<InventoryItemView> GetItem(EterBase::ItemSlot slot) const = 0;
        virtual bool IsSlotEmpty(EterBase::ItemSlot slot) const = 0;
        virtual bool IsItemLocked(EterBase::ItemSlot slot) const = 0;
        virtual void SetItemLock(EterBase::ItemSlot slot, bool locked) = 0;
        virtual uint32_t GetItemCount(EterBase::ItemVnum vnum) const = 0;
        virtual EterBase::PacketResult<void> SwapSlots(EterBase::ItemSlot from, EterBase::ItemSlot to) = 0;
        virtual void Clear() = 0;
    };
}
