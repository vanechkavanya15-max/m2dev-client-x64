#include "../StdAfx.h"
#include "IInventoryService.h"
#include "../PythonPlayer.h"
#include "../Core/EventBus.h"
#include "../Core/InventoryEvents.h"
#include "../../EterBase/LogModern.h"
#include "../Network/Handlers/ItemUseModernHandler.h"

namespace UserInterface::Services
{
    class PlayerFacade_Inventory : public IInventoryService
    {
    public:
        EterBase::PacketResult<void> SetItem(EterBase::ItemSlot slot, const InventoryItemView& item) override
        {
            TItemPos pos(INVENTORY, slot.value());
            TItemData data{};
            data.vnum = item.vnum.value();
            data.count = static_cast<uint8_t>(item.count);
            data.flags = 0;
            data.anti_flags = 0;

            for (size_t i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
                data.alSockets[i] = item.sockets[i];

            for (size_t i = 0; i < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++i)
            {
                data.aAttr[i].bType = static_cast<uint8_t>(item.attrTypes[i]);
                data.aAttr[i].sValue = item.attrValues[i];
            }

            CPythonPlayer::Instance().SetItemData(pos, data);

            EterBase::ModernLogger::Debug("PlayerFacade_Inventory::SetItem slot: {}, vnum: {}", slot.value(), item.vnum.value());
            
            Core::EventBus::GetInstance().Publish(Core::InventoryEvents::ItemAcquired(item.vnum, slot, item.count));
            Core::EventBus::GetInstance().Publish(Network::Handlers::InventoryRefreshEvent());

            return {};
        }

        EterBase::PacketResult<void> RemoveItem(EterBase::ItemSlot slot) override
        {
            TItemPos pos(INVENTORY, slot.value());
            TItemData data{}; // Empty item
            CPythonPlayer::Instance().SetItemData(pos, data);
            
            EterBase::ModernLogger::Debug("PlayerFacade_Inventory::RemoveItem slot: {}", slot.value());
            Core::EventBus::GetInstance().Publish(Network::Handlers::InventoryRefreshEvent());

            return {};
        }

        std::optional<InventoryItemView> GetItem(EterBase::ItemSlot slot) const override
        {
            TItemPos pos(INVENTORY, slot.value());
            const TItemData* data = CPythonPlayer::Instance().GetItemData(pos);
            
            if (!data || data->vnum == 0)
                return std::nullopt;

            InventoryItemView view{};
            view.slot = slot;
            view.vnum = EterBase::ItemVnum(data->vnum);
            view.count = data->count;
            
            for (size_t i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
                view.sockets[i] = data->alSockets[i];

            for (size_t i = 0; i < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++i)
            {
                view.attrTypes[i] = data->aAttr[i].bType;
                view.attrValues[i] = data->aAttr[i].sValue;
            }
            view.isLocked = false; // Legacy system doesn't natively expose lock state per item easily

            return view;
        }

        bool IsSlotEmpty(EterBase::ItemSlot slot) const override
        {
            TItemPos pos(INVENTORY, slot.value());
            return CPythonPlayer::Instance().GetItemIndex(pos) == 0;
        }

        bool IsItemLocked(EterBase::ItemSlot slot) const override
        {
            // Note: CPythonPlayer handles locks separately, returning false for now
            return false;
        }

        void SetItemLock(EterBase::ItemSlot slot, bool locked) override
        {
            // Placeholder: CPythonPlayer doesn't have a direct SetItemLock on item data
        }

        uint32_t GetItemCount(EterBase::ItemVnum vnum) const override
        {
            return CPythonPlayer::Instance().GetItemCountByVnum(vnum.value());
        }

        EterBase::PacketResult<void> SwapSlots(EterBase::ItemSlot from, EterBase::ItemSlot to) override
        {
            TItemPos posFrom(INVENTORY, from.value());
            TItemPos posTo(INVENTORY, to.value());
            
            CPythonPlayer::Instance().MoveItemData(posFrom, posTo);

            EterBase::ModernLogger::Debug("PlayerFacade_Inventory::SwapSlots {} -> {}", from.value(), to.value());
            Core::EventBus::GetInstance().Publish(Network::Handlers::InventoryRefreshEvent());

            return {};
        }

        void Clear() override
        {
            // Empty all inventory slots
            for (uint16_t i = 0; i < c_Inventory_Count; ++i)
            {
                TItemPos pos(INVENTORY, i);
                TItemData data{};
                CPythonPlayer::Instance().SetItemData(pos, data);
            }
            EterBase::ModernLogger::Info("PlayerFacade_Inventory::Clear() inventory cleared.");
            Core::EventBus::GetInstance().Publish(Network::Handlers::InventoryRefreshEvent());
        }
    };
    
    // Factory method or external registration can be added here if needed
}
