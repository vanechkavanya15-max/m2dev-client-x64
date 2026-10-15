#pragma once

#include "IInventoryService.h"
#include <unordered_map>
#include <optional>

namespace UserInterface::Services
{
    class InventoryService : public IInventoryService
    {
    public:
        static InventoryService& Instance()
        {
            static InventoryService s_instance;
            return s_instance;
        }

        InventoryService() = default;
        ~InventoryService() override = default;

        EterBase::PacketResult<void> SetItem(EterBase::ItemSlot slot, const InventoryItemView& item) override;
        EterBase::PacketResult<void> RemoveItem(EterBase::ItemSlot slot) override {
            m_items.erase(slot.value());
            return {};
        }
        std::optional<InventoryItemView> GetItem(EterBase::ItemSlot slot) const override {
            auto it = m_items.find(slot.value());
            if (it != m_items.end()) return it->second;
            return std::nullopt;
        }
        bool IsSlotEmpty(EterBase::ItemSlot slot) const override {
            return m_items.find(slot.value()) == m_items.end();
        }
        bool IsItemLocked(EterBase::ItemSlot slot) const override {
            auto it = m_locked.find(slot.value());
            return it != m_locked.end() && it->second;
        }
        void SetItemLock(EterBase::ItemSlot slot, bool locked) override {
            m_locked[slot.value()] = locked;
        }
        EterBase::PacketResult<void> SwapSlots(EterBase::ItemSlot from, EterBase::ItemSlot to) override {
            std::swap(m_items[from.value()], m_items[to.value()]);
            return {};
        }
        uint32_t GetItemCount(EterBase::ItemVnum vnum) const override {
            uint32_t count = 0;
            for (const auto& [_, item] : m_items) {
                if (item.vnum == vnum) count += item.count;
            }
            return count;
        }
        void Clear() override {
            m_items.clear();
            m_locked.clear();
        }

    private:
        std::unordered_map<uint32_t, InventoryItemView> m_items;
        std::unordered_map<uint32_t, bool> m_locked;
    };
}
