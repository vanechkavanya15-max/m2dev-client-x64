#include "../StdAfx.h"
#include "ISpecialInventoryService.h"
#include "Core/EventBus.h"
#include "EterBase/LogModern.h"

#include <array>
#include <unordered_map>
#include <optional>

namespace UserInterface::Services
{
    // Local definition to avoid including GameType.h which brings in Windows-specific dependencies
    constexpr uint8_t LOCAL_DS_DECK_MAX_NUM = 2;

    /**
     * @brief Zdarzenie emitowane po zmianie stanu aktywacji decku Smoczej Alchemii.
     */
    struct DragonSoulDeckStateChangedEvent : public Core::IEvent
    {
        uint8_t deck;
        bool active;

        DragonSoulDeckStateChangedEvent(uint8_t deck, bool active)
            : deck(deck), active(active) {}
    };

    /**
     * @brief Implementacja serwisu specjalnego ekwipunku dla stanow decku Smoczej Alchemii.
     */
    class DragonSoulDeckState final : public ISpecialInventoryService
    {
    public:
        DragonSoulDeckState()
        {
            Clear();
        }

        ~DragonSoulDeckState() override = default;

        EterBase::PacketResult<void> SetBeltItem(EterBase::ItemSlot slot, const InventoryItemView& item) override
        {
            m_beltItems[slot.value()] = item;
            EterBase::ModernLogger::Debug("Belt item set at slot {}", slot.value());
            return {};
        }

        EterBase::PacketResult<void> RemoveBeltItem(EterBase::ItemSlot slot) override
        {
            if (m_beltItems.erase(slot.value()) > 0)
            {
                EterBase::ModernLogger::Debug("Belt item removed from slot {}", slot.value());
            }
            return {};
        }

        std::optional<InventoryItemView> GetBeltItem(EterBase::ItemSlot slot) const override
        {
            if (auto it = m_beltItems.find(slot.value()); it != m_beltItems.end())
            {
                return it->second;
            }
            return std::nullopt;
        }

        EterBase::PacketResult<void> SetDragonSoulItem(uint8_t deck, EterBase::ItemSlot slot, const InventoryItemView& item) override
        {
            if (deck >= LOCAL_DS_DECK_MAX_NUM)
            {
                EterBase::ModernLogger::Error("Invalid DragonSoul deck index: {}", deck);
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            m_dsItems[deck][slot.value()] = item;
            EterBase::ModernLogger::Debug("DragonSoul item set at deck {}, slot {}", deck, slot.value());
            return {};
        }

        EterBase::PacketResult<void> RemoveDragonSoulItem(uint8_t deck, EterBase::ItemSlot slot) override
        {
            if (deck >= LOCAL_DS_DECK_MAX_NUM)
            {
                EterBase::ModernLogger::Error("Invalid DragonSoul deck index: {}", deck);
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            if (m_dsItems[deck].erase(slot.value()) > 0)
            {
                EterBase::ModernLogger::Debug("DragonSoul item removed from deck {}, slot {}", deck, slot.value());
            }
            return {};
        }

        void SetDragonSoulDeckActive(uint8_t deck, bool active) override
        {
            if (deck >= LOCAL_DS_DECK_MAX_NUM)
            {
                EterBase::ModernLogger::Error("Invalid DragonSoul deck index for activation: {}", deck);
                return;
            }

            if (m_activeDecks[deck] != active)
            {
                m_activeDecks[deck] = active;
                EterBase::ModernLogger::Info("DragonSoul deck {} state changed to {}", deck, active);

                DragonSoulDeckStateChangedEvent event{deck, active};
                Core::EventBus::GetInstance().Publish(event);
            }
        }

        bool IsDragonSoulDeckActive(uint8_t deck) const override
        {
            if (deck >= LOCAL_DS_DECK_MAX_NUM)
            {
                return false;
            }
            return m_activeDecks[deck];
        }

        void Clear() override
        {
            m_beltItems.clear();
            for (auto& items : m_dsItems)
            {
                items.clear();
            }
            m_activeDecks.fill(false);
            EterBase::ModernLogger::Debug("DragonSoulDeckState cleared.");
        }

    private:
        std::unordered_map<uint16_t, InventoryItemView> m_beltItems;
        std::array<std::unordered_map<uint16_t, InventoryItemView>, LOCAL_DS_DECK_MAX_NUM> m_dsItems;
        std::array<bool, LOCAL_DS_DECK_MAX_NUM> m_activeDecks;
    };
}
