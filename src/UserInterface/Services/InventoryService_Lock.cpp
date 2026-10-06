#include "../StdAfx.h"
#include "IInventoryService.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"

#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace UserInterface::Services
{
    /**
     * @brief Zdarzenie publikowane gdy zmienia sie stan blokady slotu (np. podczas handlu/ulepszania).
     */
    struct InventorySlotLockEvent : public Core::IEvent
    {
        EterBase::ItemSlot slot;
        bool isLocked;

        InventorySlotLockEvent(EterBase::ItemSlot slot, bool locked)
            : slot(slot), isLocked(locked) {}
    };

    /**
     * @brief Implementacja serwisu ekwipunku skupiajaca sie na logice blokowania slotow (InventoryLockState).
     */
    class InventoryService final : public IInventoryService
    {
    public:
        InventoryService() = default;
        ~InventoryService() override = default;

        EterBase::PacketResult<void> SetItem(EterBase::ItemSlot slot, const InventoryItemView& item) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_items.contains(slot) && m_items[slot].isLocked)
            {
                EterBase::ModernLogger::Warning("Cannot SetItem on locked slot: {}", slot.value());
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            m_items[slot] = item;
            EterBase::ModernLogger::Trace("SetItem to slot: {} (Vnum: {})", slot.value(), item.vnum.value());
            return {};
        }

        EterBase::PacketResult<void> RemoveItem(EterBase::ItemSlot slot) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (auto it = m_items.find(slot); it != m_items.end())
            {
                if (it->second.isLocked)
                {
                    EterBase::ModernLogger::Warning("Cannot RemoveItem from locked slot: {}", slot.value());
                    return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
                }
                m_items.erase(it);
                EterBase::ModernLogger::Trace("Removed item from slot: {}", slot.value());
                return {};
            }
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload); // Slot was empty
        }

        std::optional<InventoryItemView> GetItem(EterBase::ItemSlot slot) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (auto it = m_items.find(slot); it != m_items.end())
            {
                return it->second;
            }
            return std::nullopt;
        }

        bool IsSlotEmpty(EterBase::ItemSlot slot) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return !m_items.contains(slot);
        }

        bool IsItemLocked(EterBase::ItemSlot slot) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (auto it = m_items.find(slot); it != m_items.end())
            {
                return it->second.isLocked;
            }
            return false;
        }

        void SetItemLock(EterBase::ItemSlot slot, bool locked) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (auto it = m_items.find(slot); it != m_items.end())
            {
                if (it->second.isLocked != locked)
                {
                    it->second.isLocked = locked;
                    EterBase::ModernLogger::Info("Item at slot {} lock state changed to: {}", slot.value(), locked);
                    
                    // Publish event for GUI update
                    Core::EventBus::GetInstance().Publish(InventorySlotLockEvent{slot, locked});
                }
            }
            else
            {
                EterBase::ModernLogger::Warning("Attempted to set lock on empty slot: {}", slot.value());
            }
        }

        uint32_t GetItemCount(EterBase::ItemVnum vnum) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            uint32_t count = 0;
            for (const auto& [slot, item] : m_items)
            {
                if (item.vnum == vnum)
                {
                    count += item.count;
                }
            }
            return count;
        }

        EterBase::PacketResult<void> SwapSlots(EterBase::ItemSlot from, EterBase::ItemSlot to) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            bool fromLocked = m_items.contains(from) && m_items[from].isLocked;
            bool toLocked = m_items.contains(to) && m_items[to].isLocked;

            if (fromLocked || toLocked)
            {
                EterBase::ModernLogger::Warning("Cannot SwapSlots with locked slots (From: {}, To: {})", from.value(), to.value());
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            auto itFrom = m_items.find(from);
            auto itTo = m_items.find(to);

            std::optional<InventoryItemView> itemFrom = (itFrom != m_items.end()) ? std::make_optional(itFrom->second) : std::nullopt;
            std::optional<InventoryItemView> itemTo = (itTo != m_items.end()) ? std::make_optional(itTo->second) : std::nullopt;

            if (itemTo.has_value())
            {
                itemTo->slot = from;
                m_items[from] = *itemTo;
            }
            else
            {
                m_items.erase(from);
            }

            if (itemFrom.has_value())
            {
                itemFrom->slot = to;
                m_items[to] = *itemFrom;
            }
            else
            {
                m_items.erase(to);
            }

            EterBase::ModernLogger::Trace("Swapped slots: {} <-> {}", from.value(), to.value());
            return {};
        }

        void Clear() override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_items.clear();
            EterBase::ModernLogger::Info("Inventory cleared.");
        }

    private:
        mutable std::mutex m_mutex;
        std::unordered_map<EterBase::ItemSlot, InventoryItemView> m_items;
    };
}
