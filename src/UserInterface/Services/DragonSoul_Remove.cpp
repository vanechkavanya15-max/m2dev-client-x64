#include "../StdAfx.h"
#include "ISpecialInventoryService.h"
#include "../Packet.h"
#include "../PythonNetworkStream.h"
#include "../../EterBase/LogModern.h"
#include "../GameType.h"
#include "../Core/EventBus.h"
#include <span>

namespace UserInterface::Services
{
    /**
     * @brief Zdejmuje smoczy kamień z wybranego decku i slotu (DragonSoul)
     * 
     * Freestanding implementation dla logiki usługowej ISpecialInventoryService.
     * SRP: Plik zajmuje się tylko i wyłącznie jedną operacją (usuwaniem DS Itemu).
     * 
     * @param deck  Identyfikator decku (np. 0, 1).
     * @param slot  Identyfikator slotu wewnątrz danego decku.
     * @return EterBase::PacketResult<void> Pusty rezultat w przypadku sukcesu lub PacketError w przypadku niepowodzenia.
     */
    EterBase::PacketResult<void> RemoveDragonSoulItemFreestanding(uint8_t deck, EterBase::ItemSlot slot)
    {
        if (deck >= DS_DECK_MAX_NUM)
        {
            EterBase::ModernLogger::Error("RemoveDragonSoulItem: Invalid deck {}. Max is {}", deck, (int)DS_DECK_MAX_NUM);
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        if (slot.value() >= c_DragonSoul_Equip_Slot_Max)
        {
            EterBase::ModernLogger::Error("RemoveDragonSoulItem: Invalid slot {}. Max is {}", slot.value(), (int)c_DragonSoul_Equip_Slot_Max);
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        const uint16_t globalCell = c_DragonSoul_Equip_Start + (deck * c_DragonSoul_Equip_Slot_Max) + slot.value();
        TItemPos pos(INVENTORY, globalCell);

#pragma pack(push, 1)
        TPacketCGItemUse itemUsePacket{};
#pragma pack(pop)
        
        itemUsePacket.header = CG::ITEM_USE;
        itemUsePacket.length = sizeof(itemUsePacket);
        itemUsePacket.pos = pos;

        std::span<const uint8_t> buffer(reinterpret_cast<const uint8_t*>(&itemUsePacket), sizeof(itemUsePacket));

        if (!CPythonNetworkStream::Instance().Send(static_cast<int>(buffer.size()), buffer.data()))
        {
            EterBase::ModernLogger::Error("RemoveDragonSoulItem: Failed to transmit ITEM_USE packet. Deck: {}, Slot: {}", deck, slot.value());
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Info("RemoveDragonSoulItem: Successfully sent ITEM_USE for DragonSoul deck {}, slot {} (global cell {})", 
            deck, slot.value(), globalCell);

        UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::NetworkPacketReceivedEvent(CG::ITEM_USE, buffer));

        return {};
    }
}
