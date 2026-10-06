#include "../StdAfx.h"
#include "ISpecialInventoryService.h"
#include "../Domain/DragonSoulContainerModel.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

#include <memory>

namespace UserInterface::Services {

    /**
     * @brief Implementacja serwisu Dragon Soul Set.
     * Zgodna z Single Responsibility Principle, obsluguje glownie zdarzenia dla Dragon Soul.
     * Uzywa DragonSoulContainerModel z warstwy domenowej do modyfikowania logiki 
     * i wysylania eventow przez EventBus bezposrednio przy zmianach wyposazenia.
     */
    class DragonSoulSetService : public ISpecialInventoryService {
    public:
        DragonSoulSetService() = default;
        ~DragonSoulSetService() override = default;

        EterBase::PacketResult<void> SetDragonSoulItem(uint8_t deck, EterBase::ItemSlot slot, const InventoryItemView& item) override {
            if (deck >= Domain::DragonSoulContainerModel::DRAGON_SOUL_DECK_MAX_NUM) {
                EterBase::ModernLogger::Error("SetDragonSoulItem: Invalid deck index {}.", deck);
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            if (slot.value() >= Domain::DragonSoulContainerModel::DRAGON_SOUL_EQUIP_SLOT_MAX) {
                EterBase::ModernLogger::Error("SetDragonSoulItem: Invalid slot index {}.", slot.value());
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            EterBase::ItemSlot absoluteSlot(deck * Domain::DragonSoulContainerModel::DRAGON_SOUL_EQUIP_SLOT_MAX + slot.value());

            auto equipResult = m_container.EquipItem(absoluteSlot, item.vnum);
            if (!equipResult) {
                EterBase::ModernLogger::Error("SetDragonSoulItem: Failed to equip item {} in slot {}.", item.vnum.value(), absoluteSlot.value());
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            EterBase::ModernLogger::Info("SetDragonSoulItem: Successfully equipped item {} to deck {}, slot {}.", item.vnum.value(), deck, slot.value());
            return {};
        }

        EterBase::PacketResult<void> RemoveDragonSoulItem(uint8_t deck, EterBase::ItemSlot slot) override {
            if (deck >= Domain::DragonSoulContainerModel::DRAGON_SOUL_DECK_MAX_NUM) {
                EterBase::ModernLogger::Error("RemoveDragonSoulItem: Invalid deck index {}.", deck);
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            if (slot.value() >= Domain::DragonSoulContainerModel::DRAGON_SOUL_EQUIP_SLOT_MAX) {
                EterBase::ModernLogger::Error("RemoveDragonSoulItem: Invalid slot index {}.", slot.value());
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            EterBase::ItemSlot absoluteSlot(deck * Domain::DragonSoulContainerModel::DRAGON_SOUL_EQUIP_SLOT_MAX + slot.value());

            auto unequipResult = m_container.UnequipItem(absoluteSlot);
            if (!unequipResult) {
                EterBase::ModernLogger::Error("RemoveDragonSoulItem: Failed to unequip item from slot {}.", absoluteSlot.value());
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            EterBase::ModernLogger::Info("RemoveDragonSoulItem: Successfully removed item from deck {}, slot {}.", deck, slot.value());
            return {};
        }

        void SetDragonSoulDeckActive(uint8_t deck, bool active) override {
            if (deck >= Domain::DragonSoulContainerModel::DRAGON_SOUL_DECK_MAX_NUM) {
                EterBase::ModernLogger::Error("SetDragonSoulDeckActive: Invalid deck index {}.", deck);
                return;
            }

            if (active) {
                auto result = m_container.SetActiveDeck(deck);
                if (!result) {
                    EterBase::ModernLogger::Error("SetDragonSoulDeckActive: Failed to set active deck to {}.", deck);
                    return;
                }
            }

            m_container.SetActive(active);
            EterBase::ModernLogger::Info("SetDragonSoulDeckActive: Deck {} is now {}.", deck, active ? "active" : "inactive");
        }

        bool IsDragonSoulDeckActive(uint8_t deck) const override {
            if (!m_container.IsActive()) {
                return false;
            }
            return m_container.GetActiveDeck() == deck;
        }

        void Clear() override {
            m_container.Clear();
            EterBase::ModernLogger::Debug("DragonSoulSetService: Cleared all items.");
        }

        EterBase::PacketResult<void> SetBeltItem(EterBase::ItemSlot slot, const InventoryItemView& item) override {
            EterBase::ModernLogger::Error("SetBeltItem is not supported in DragonSoulSetService.");
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
        }

        EterBase::PacketResult<void> RemoveBeltItem(EterBase::ItemSlot slot) override {
            EterBase::ModernLogger::Error("RemoveBeltItem is not supported in DragonSoulSetService.");
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
        }

        std::optional<InventoryItemView> GetBeltItem(EterBase::ItemSlot slot) const override {
            EterBase::ModernLogger::Error("GetBeltItem is not supported in DragonSoulSetService.");
            return std::nullopt;
        }

    private:
        Domain::DragonSoulContainerModel m_container;
    };

    std::unique_ptr<ISpecialInventoryService> CreateDragonSoulSetService() {
        return std::make_unique<DragonSoulSetService>();
    }
}
