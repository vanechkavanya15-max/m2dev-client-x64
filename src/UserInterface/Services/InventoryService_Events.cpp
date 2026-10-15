#include "../StdAfx.h"
#include "IInventoryService.h"
#include "../Core/EventBus.h"
#include "../Core/InventoryEvents.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include <unordered_map>
#include <optional>
#include <algorithm>
#include <numeric>

namespace UserInterface::Services
{
    class InventoryEventSync : public IInventoryService
    {
    public:
        virtual ~InventoryEventSync() = default;

        EterBase::PacketResult<void> SetItem(EterBase::ItemSlot slot, const InventoryItemView& item) override
        {
            m_items[slot] = item;
            
            EterBase::ModernLogger::Info("InventoryEventSync: Item set in slot {}, VNUM {}", static_cast<uint16_t>(slot.value()), static_cast<uint32_t>(item.vnum.value()));
            
            Core::EventBus::GetInstance().Publish(
                Core::InventoryEvents::ItemAcquired(item.vnum, slot, item.count)
            );
            
            return {};
        }

        EterBase::PacketResult<void> RemoveItem(EterBase::ItemSlot slot) override
        {
            auto it = m_items.find(slot);
            if (it != m_items.end())
            {
                auto item = it->second;
                m_items.erase(it);
                
                EterBase::ModernLogger::Info("InventoryEventSync: Item removed from slot {}, VNUM {}", static_cast<uint16_t>(slot.value()), static_cast<uint32_t>(item.vnum.value()));

                Core::EventBus::GetInstance().Publish(
                    Core::InventoryEvents::ItemDropped(item.vnum, slot, item.count)
                );
            }
            
            return {};
        }

        std::optional<InventoryItemView> GetItem(EterBase::ItemSlot slot) const override
        {
            if (auto it = m_items.find(slot); it != m_items.end())
            {
                return it->second;
            }
            return std::nullopt;
        }

        bool IsSlotEmpty(EterBase::ItemSlot slot) const override
        {
            return m_items.find(slot) == m_items.end();
        }

        bool IsItemLocked(EterBase::ItemSlot slot) const override
        {
            if (auto it = m_items.find(slot); it != m_items.end())
            {
                return it->second.isLocked;
            }
            return false;
        }

        void SetItemLock(EterBase::ItemSlot slot, bool locked) override
        {
            if (auto it = m_items.find(slot); it != m_items.end())
            {
                it->second.isLocked = locked;
                EterBase::ModernLogger::Debug("InventoryEventSync: Lock state of slot {} set to {}", static_cast<uint16_t>(slot.value()), locked);
            }
        }

        uint32_t GetItemCount(EterBase::ItemVnum vnum) const override
        {
            uint32_t totalCount = 0;
            for (const auto& [slot, item] : m_items)
            {
                if (item.vnum == vnum)
                {
                    totalCount += item.count;
                }
            }
            return totalCount;
        }

        EterBase::PacketResult<void> SwapSlots(EterBase::ItemSlot from, EterBase::ItemSlot to) override
        {
            auto itFrom = m_items.find(from);
            auto itTo = m_items.find(to);

            InventoryItemView tempFrom;
            bool hasFrom = false;
            
            InventoryItemView tempTo;
            bool hasTo = false;

            if (itFrom != m_items.end())
            {
                tempFrom = itFrom->second;
                hasFrom = true;
                tempFrom.slot = to;
            }

            if (itTo != m_items.end())
            {
                tempTo = itTo->second;
                hasTo = true;
                tempTo.slot = from;
                m_items[from] = tempTo;
            }
            else
            {
                m_items.erase(from);
            }

            if (hasFrom)
            {
                m_items[to] = tempFrom;
            }
            else
            {
                m_items.erase(to);
            }

            if (hasFrom)
            {
                Core::EventBus::GetInstance().Publish(Core::InventoryEvents::ItemDropped(tempFrom.vnum, from, tempFrom.count));
                Core::EventBus::GetInstance().Publish(Core::InventoryEvents::ItemAcquired(tempFrom.vnum, to, tempFrom.count));
            }
            if (hasTo)
            {
                Core::EventBus::GetInstance().Publish(Core::InventoryEvents::ItemDropped(tempTo.vnum, to, tempTo.count));
                Core::EventBus::GetInstance().Publish(Core::InventoryEvents::ItemAcquired(tempTo.vnum, from, tempTo.count));
            }

            EterBase::ModernLogger::Info("InventoryEventSync: Swapped slots {} and {}", static_cast<uint16_t>(from.value()), static_cast<uint16_t>(to.value()));

            return {};
        }

        void Clear() override
        {
            m_items.clear();
            EterBase::ModernLogger::Info("InventoryEventSync: Inventory cleared");
        }

    private:
        std::unordered_map<EterBase::ItemSlot, InventoryItemView> m_items;
    };
}
