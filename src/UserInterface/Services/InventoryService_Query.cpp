#include "../StdAfx.h"
#include "IInventoryService.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../Core/EventBus.h"
#include "../Core/InventoryEvents.h"

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <format>
#include <optional>

namespace UserInterface::Services
{
    class InventoryService final : public IInventoryService
    {
    public:
        InventoryService() : m_capacity(90) {
            m_slots.resize(m_capacity);
        }

        EterBase::PacketResult<void> SetItem(EterBase::ItemSlot slot, const InventoryItemView& item) override
        {
            if (slot.get() >= m_capacity)
            {
                EterBase::ModernLogger::Error("InventoryService::SetItem: Slot {} out of range", slot.get());
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            if (m_slots[slot.get()].has_value())
            {
                auto oldVnum = m_slots[slot.get()]->vnum;
                auto oldCount = m_slots[slot.get()]->count;
                if (m_itemCounts[oldVnum] >= oldCount)
                {
                    m_itemCounts[oldVnum] -= oldCount;
                }
                else
                {
                    m_itemCounts[oldVnum] = 0;
                }
            }

            m_slots[slot.get()] = item;
            m_itemCounts[item.vnum] += item.count;

            EterBase::ModernLogger::Trace("InventoryService::SetItem: Slot {} set to vnum {} (count {})", 
                slot.get(), item.vnum.get(), item.count);

            Core::EventBus::GetInstance().Publish(Core::InventoryEvents::ItemAcquired(item.vnum, slot, item.count));

            return {};
        }

        EterBase::PacketResult<void> RemoveItem(EterBase::ItemSlot slot) override
        {
            if (slot.get() >= m_capacity)
            {
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            if (m_slots[slot.get()].has_value())
            {
                auto oldVnum = m_slots[slot.get()]->vnum;
                auto oldCount = m_slots[slot.get()]->count;
                if (m_itemCounts[oldVnum] >= oldCount)
                {
                    m_itemCounts[oldVnum] -= oldCount;
                }
                else
                {
                    m_itemCounts[oldVnum] = 0;
                }

                m_slots[slot.get()].reset();
                EterBase::ModernLogger::Trace("InventoryService::RemoveItem: Slot {} cleared", slot.get());
                
                Core::EventBus::GetInstance().Publish(Core::InventoryEvents::ItemDropped(oldVnum, slot, oldCount));
            }
            return {};
        }

        std::optional<InventoryItemView> GetItem(EterBase::ItemSlot slot) const override
        {
            if (slot.get() >= m_capacity)
            {
                return std::nullopt;
            }
            return m_slots[slot.get()];
        }

        bool IsSlotEmpty(EterBase::ItemSlot slot) const override
        {
            if (slot.get() >= m_capacity)
            {
                return true;
            }
            return !m_slots[slot.get()].has_value();
        }

        bool IsItemLocked(EterBase::ItemSlot slot) const override
        {
            if (slot.get() >= m_capacity)
            {
                return false;
            }
            return m_slots[slot.get()].has_value() && m_slots[slot.get()]->isLocked;
        }

        void SetItemLock(EterBase::ItemSlot slot, bool locked) override
        {
            if (slot.get() >= m_capacity)
            {
                return;
            }
            
            if (m_slots[slot.get()].has_value())
            {
                m_slots[slot.get()]->isLocked = locked;
            }
        }

        uint32_t GetItemCount(EterBase::ItemVnum vnum) const override
        {
            auto it = m_itemCounts.find(vnum);
            if (it != m_itemCounts.end())
            {
                return it->second;
            }
            return 0;
        }

        EterBase::PacketResult<void> SwapSlots(EterBase::ItemSlot from, EterBase::ItemSlot to) override
        {
            if (from.get() >= m_capacity || to.get() >= m_capacity)
            {
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            std::swap(m_slots[from.get()], m_slots[to.get()]);
            
            if (m_slots[from.get()].has_value())
            {
                m_slots[from.get()]->slot = from;
            }
            if (m_slots[to.get()].has_value())
            {
                m_slots[to.get()]->slot = to;
            }

            return {};
        }

        void Clear() override
        {
            m_slots.clear();
            m_slots.resize(m_capacity);
            m_itemCounts.clear();
        }

    private:
        uint16_t m_capacity;
        std::vector<std::optional<InventoryItemView>> m_slots;
        std::unordered_map<EterBase::ItemVnum, uint32_t> m_itemCounts;
    };
}
