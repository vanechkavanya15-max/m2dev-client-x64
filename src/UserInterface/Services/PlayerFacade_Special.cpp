#include "../StdAfx.h"
#include "ISpecialInventoryService.h"
#include "../Domain/BeltInventoryModel.h"
#include "../Domain/DragonSoulContainerModel.h"
#include "../GameType.h"
#include "../../EterBase/LogModern.h"

namespace UserInterface::Services
{
    class PlayerFacadeSpecialBridge : public ISpecialInventoryService
    {
    public:
        PlayerFacadeSpecialBridge() 
            : m_beltModel(EterBase::ItemSlot{static_cast<uint16_t>(c_Belt_Inventory_Slot_Count)})
        {
        }

        virtual ~PlayerFacadeSpecialBridge() = default;

        EterBase::PacketResult<void> SetBeltItem(EterBase::ItemSlot slot, const InventoryItemView& item) override
        {
            Domain::BeltItemData beltItem{ item.vnum, item.count };
            auto result = m_beltModel.setItem(slot, beltItem);
            
            if (!result)
            {
                EterBase::ModernLogger::Error("PlayerFacadeSpecialBridge: Failed to set belt item at slot {}.", slot.get());
                return std::unexpected(EterBase::PacketError::MalformedPayload);
            }

            UserInterface::Core::EventBus::GetInstance().Publish(Domain::BeltInventorySlotUpdateEvent{slot});
            EterBase::ModernLogger::Info("PlayerFacadeSpecialBridge: Successfully set belt item at slot {}.", slot.get());
            return {};
        }

        EterBase::PacketResult<void> RemoveBeltItem(EterBase::ItemSlot slot) override
        {
            auto result = m_beltModel.clearItem(slot);
            
            if (!result)
            {
                EterBase::ModernLogger::Error("PlayerFacadeSpecialBridge: Failed to remove belt item at slot {}.", slot.get());
                return std::unexpected(EterBase::PacketError::MalformedPayload);
            }

            UserInterface::Core::EventBus::GetInstance().Publish(Domain::BeltInventorySlotUpdateEvent{slot});
            EterBase::ModernLogger::Info("PlayerFacadeSpecialBridge: Successfully removed belt item at slot {}.", slot.get());
            return {};
        }

        std::optional<InventoryItemView> GetBeltItem(EterBase::ItemSlot slot) const override
        {
            auto result = m_beltModel.getItem(slot);
            if (!result)
            {
                return std::nullopt;
            }

            InventoryItemView view{};
            view.slot = slot;
            view.vnum = result->vnum;
            view.count = result->count;
            view.isLocked = false;
            
            for (size_t i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
                view.sockets[i] = 0;
            for (size_t i = 0; i < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++i)
            {
                view.attrTypes[i] = 0;
                view.attrValues[i] = 0;
            }
            
            return view;
        }

        EterBase::PacketResult<void> SetDragonSoulItem(uint8_t deck, EterBase::ItemSlot slot, const InventoryItemView& item) override
        {
            uint16_t absSlot = deck * ::Domain::DragonSoulContainerModel::DRAGON_SOUL_EQUIP_SLOT_MAX + slot.get();
            auto result = m_dsModel.EquipItem(EterBase::ItemSlot{absSlot}, item.vnum);
            
            if (!result)
            {
                EterBase::ModernLogger::Error("PlayerFacadeSpecialBridge: Failed to set Dragon Soul item at deck {}, slot {}.", deck, slot.get());
                return std::unexpected(EterBase::PacketError::MalformedPayload);
            }

            UserInterface::Core::EventBus::GetInstance().Publish(::Domain::DragonSoulEquipChangedEvent{EterBase::ItemSlot{absSlot}, item.vnum});
            EterBase::ModernLogger::Info("PlayerFacadeSpecialBridge: Successfully set Dragon Soul item at deck {}, slot {}.", deck, slot.get());
            return {};
        }

        EterBase::PacketResult<void> RemoveDragonSoulItem(uint8_t deck, EterBase::ItemSlot slot) override
        {
            uint16_t absSlot = deck * ::Domain::DragonSoulContainerModel::DRAGON_SOUL_EQUIP_SLOT_MAX + slot.get();
            auto result = m_dsModel.UnequipItem(EterBase::ItemSlot{absSlot});
            
            if (!result)
            {
                EterBase::ModernLogger::Error("PlayerFacadeSpecialBridge: Failed to remove Dragon Soul item at deck {}, slot {}.", deck, slot.get());
                return std::unexpected(EterBase::PacketError::MalformedPayload);
            }

            UserInterface::Core::EventBus::GetInstance().Publish(::Domain::DragonSoulEquipChangedEvent{EterBase::ItemSlot{absSlot}, std::nullopt});
            EterBase::ModernLogger::Info("PlayerFacadeSpecialBridge: Successfully removed Dragon Soul item at deck {}, slot {}.", deck, slot.get());
            return {};
        }

        void SetDragonSoulDeckActive(uint8_t deck, bool active) override
        {
            auto result = m_dsModel.SetActiveDeck(deck);
            if (!result)
            {
                EterBase::ModernLogger::Error("PlayerFacadeSpecialBridge: Failed to set Dragon Soul active deck {}.", deck);
                return;
            }
            
            m_dsModel.SetActive(active);
            UserInterface::Core::EventBus::GetInstance().Publish(::Domain::DragonSoulDeckChangedEvent{deck});
            EterBase::ModernLogger::Info("PlayerFacadeSpecialBridge: Dragon Soul deck {} active state set to {}.", deck, active);
        }

        bool IsDragonSoulDeckActive(uint8_t deck) const override
        {
            return (m_dsModel.GetActiveDeck() == deck) && m_dsModel.IsActive();
        }

        void Clear() override
        {
            for (uint16_t i = 0; i < c_Belt_Inventory_Slot_Count; ++i)
            {
                auto itemResult = m_beltModel.getItem(EterBase::ItemSlot{i});
                if (itemResult)
                {
                    (void)m_beltModel.clearItem(EterBase::ItemSlot{i});
                }
            }
            
            m_dsModel.Clear();
            EterBase::ModernLogger::Info("PlayerFacadeSpecialBridge: Cleared Special Inventory.");
        }

    private:
        Domain::BeltInventoryModel m_beltModel;
        ::Domain::DragonSoulContainerModel m_dsModel;
    };

    std::unique_ptr<ISpecialInventoryService> CreateSpecialInventoryService()
    {
        return std::make_unique<PlayerFacadeSpecialBridge>();
    }
}
