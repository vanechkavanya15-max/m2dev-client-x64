#pragma once

#include "ISpecialInventoryService.h"
#include <unordered_map>
#include <optional>

namespace UserInterface::Services
{
    class SpecialInventoryService : public ISpecialInventoryService
    {
    public:
        SpecialInventoryService() = default;
        ~SpecialInventoryService() override = default;

        EterBase::PacketResult<void> SetBeltItem(EterBase::ItemSlot slot, const InventoryItemView& item) override
        {
            m_beltItems[slot.value()] = item;
            return {};
        }

        EterBase::PacketResult<void> RemoveBeltItem(EterBase::ItemSlot slot) override
        {
            m_beltItems.erase(slot.value());
            return {};
        }

        std::optional<InventoryItemView> GetBeltItem(EterBase::ItemSlot slot) const override
        {
            auto it = m_beltItems.find(slot.value());
            if (it != m_beltItems.end()) return it->second;
            return std::nullopt;
        }

        EterBase::PacketResult<void> SetDragonSoulItem(uint8_t deck, EterBase::ItemSlot slot, const InventoryItemView& item) override
        {
            uint32_t key = (static_cast<uint32_t>(deck) << 16) | slot.value();
            m_dragonSoulItems[key] = item;
            return {};
        }

        EterBase::PacketResult<void> RemoveDragonSoulItem(uint8_t deck, EterBase::ItemSlot slot) override
        {
            uint32_t key = (static_cast<uint32_t>(deck) << 16) | slot.value();
            m_dragonSoulItems.erase(key);
            return {};
        }

        void SetDragonSoulDeckActive(uint8_t deck, bool active) override
        {
            m_activeDecks[deck] = active;
        }

        bool IsDragonSoulDeckActive(uint8_t deck) const override
        {
            auto it = m_activeDecks.find(deck);
            return it != m_activeDecks.end() && it->second;
        }

        void Clear() override
        {
            m_beltItems.clear();
            m_dragonSoulItems.clear();
            m_activeDecks.clear();
        }

    private:
        std::unordered_map<uint16_t, InventoryItemView> m_beltItems;
        std::unordered_map<uint32_t, InventoryItemView> m_dragonSoulItems;
        std::unordered_map<uint8_t, bool> m_activeDecks;
    };
}
