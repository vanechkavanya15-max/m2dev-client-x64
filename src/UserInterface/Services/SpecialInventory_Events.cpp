#include "../StdAfx.h"
#include "ISpecialInventoryService.h"
#include "../Core/EventBus.h"
#include "EterBase/LogModern.h"

#include <unordered_map>

namespace UserInterface::Services
{
    // ============================================================================
    // Events
    // ============================================================================

    struct BeltItemSetEvent : public Core::IEvent
    {
        EterBase::ItemSlot slot;
        InventoryItemView item;

        BeltItemSetEvent(EterBase::ItemSlot slot, const InventoryItemView& item)
            : slot(slot), item(item) {}
    };

    struct BeltItemRemovedEvent : public Core::IEvent
    {
        EterBase::ItemSlot slot;

        explicit BeltItemRemovedEvent(EterBase::ItemSlot slot)
            : slot(slot) {}
    };

    struct DragonSoulItemSetEvent : public Core::IEvent
    {
        uint8_t deck;
        EterBase::ItemSlot slot;
        InventoryItemView item;

        DragonSoulItemSetEvent(uint8_t deck, EterBase::ItemSlot slot, const InventoryItemView& item)
            : deck(deck), slot(slot), item(item) {}
    };

    struct DragonSoulItemRemovedEvent : public Core::IEvent
    {
        uint8_t deck;
        EterBase::ItemSlot slot;

        DragonSoulItemRemovedEvent(uint8_t deck, EterBase::ItemSlot slot)
            : deck(deck), slot(slot) {}
    };

    struct DragonSoulDeckActiveEvent : public Core::IEvent
    {
        uint8_t deck;
        bool active;

        DragonSoulDeckActiveEvent(uint8_t deck, bool active)
            : deck(deck), active(active) {}
    };
}

    // ============================================================================
    // Service Implementation
    // ============================================================================

namespace UserInterface::Services
{
    class SpecialInventoryService_Events final : public ISpecialInventoryService
    {
    public:
        SpecialInventoryService_Events() = default;
        ~SpecialInventoryService_Events() override = default;

        EterBase::PacketResult<void> SetBeltItem(EterBase::ItemSlot slot, const InventoryItemView& item) override
        {
            if (slot.value() >= 100) // Arbitrary validation for now to return an error if out of bounds (can be adjusted)
            {
                EterBase::ModernLogger::Error("SpecialInventoryService: Belt slot {} is out of range", slot.value());
                return std::unexpected(EterBase::PacketError::BufferUnderflow); // Just a placeholder error
            }

            m_beltItems[slot.value()] = item;
            
            EterBase::ModernLogger::Info("SpecialInventoryService: Set belt item at slot {} (vnum {})", slot.value(), item.vnum.value());
            
            Core::EventBus::GetInstance().Publish(BeltItemSetEvent(slot, item));
            return {};
        }

        EterBase::PacketResult<void> RemoveBeltItem(EterBase::ItemSlot slot) override
        {
            auto it = m_beltItems.find(slot.value());
            if (it == m_beltItems.end())
            {
                EterBase::ModernLogger::Warning("SpecialInventoryService: Cannot remove belt item at slot {}, slot is empty", slot.value());
                return std::unexpected(EterBase::PacketError::BufferUnderflow); // Placeholder error
            }

            m_beltItems.erase(it);
            
            EterBase::ModernLogger::Info("SpecialInventoryService: Removed belt item at slot {}", slot.value());
            
            Core::EventBus::GetInstance().Publish(BeltItemRemovedEvent(slot));
            return {};
        }

        std::optional<InventoryItemView> GetBeltItem(EterBase::ItemSlot slot) const override
        {
            auto it = m_beltItems.find(slot.value());
            if (it != m_beltItems.end())
            {
                return it->second;
            }
            return std::nullopt;
        }

        EterBase::PacketResult<void> SetDragonSoulItem(uint8_t deck, EterBase::ItemSlot slot, const InventoryItemView& item) override
        {
            // Simple validation
            if (deck > 1) {
                EterBase::ModernLogger::Error("SpecialInventoryService: Dragon Soul deck {} is invalid", deck);
                return std::unexpected(EterBase::PacketError::BufferUnderflow);
            }

            uint32_t key = (static_cast<uint32_t>(deck) << 16) | slot.value();
            m_dragonSoulItems[key] = item;
            
            EterBase::ModernLogger::Info("SpecialInventoryService: Set Dragon Soul item at deck {}, slot {} (vnum {})", deck, slot.value(), item.vnum.value());
            Core::EventBus::GetInstance().Publish(DragonSoulItemSetEvent(deck, slot, item));
            
            return {};
        }

        EterBase::PacketResult<void> RemoveDragonSoulItem(uint8_t deck, EterBase::ItemSlot slot) override
        {
            uint32_t key = (static_cast<uint32_t>(deck) << 16) | slot.value();
            auto it = m_dragonSoulItems.find(key);
            
            if (it == m_dragonSoulItems.end()) {
                EterBase::ModernLogger::Warning("SpecialInventoryService: Cannot remove Dragon Soul item at deck {}, slot {}, slot is empty", deck, slot.value());
                return std::unexpected(EterBase::PacketError::BufferUnderflow);
            }

            m_dragonSoulItems.erase(it);
            
            EterBase::ModernLogger::Info("SpecialInventoryService: Removed Dragon Soul item at deck {}, slot {}", deck, slot.value());
            Core::EventBus::GetInstance().Publish(DragonSoulItemRemovedEvent(deck, slot));
            
            return {};
        }

        void SetDragonSoulDeckActive(uint8_t deck, bool active) override
        {
            if (m_activeDecks[deck] != active) {
                m_activeDecks[deck] = active;
                EterBase::ModernLogger::Info("SpecialInventoryService: Set Dragon Soul deck {} active state to {}", deck, active);
                Core::EventBus::GetInstance().Publish(DragonSoulDeckActiveEvent(deck, active));
            }
        }

        bool IsDragonSoulDeckActive(uint8_t deck) const override
        {
            auto it = m_activeDecks.find(deck);
            if (it != m_activeDecks.end()) {
                return it->second;
            }
            return false;
        }

        void Clear() override
        {
            m_beltItems.clear();
            m_dragonSoulItems.clear();
            m_activeDecks.clear();
            EterBase::ModernLogger::Info("SpecialInventoryService: Cleared all special inventory state");
        }

    private:
        std::unordered_map<uint16_t, InventoryItemView> m_beltItems;
        std::unordered_map<uint32_t, InventoryItemView> m_dragonSoulItems; // Key: (deck << 16) | slot
        std::unordered_map<uint8_t, bool> m_activeDecks;
    };
}
