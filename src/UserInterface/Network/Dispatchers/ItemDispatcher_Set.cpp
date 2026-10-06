#include "../../StdAfx.h"
#include <cstdint>
#include <span>
#include <algorithm>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Packet.h"
#include "../../Services/IInventoryService.h"
#include "../../Core/EventBus.h"
#include "../../Core/InventoryEvents.h"

namespace Network::Dispatchers
{
    /**
     * @brief Odbiera pakiet ITEM_SET i przekazuje dane do IInventoryService.
     *
     * @param buffer Bufor bajtow zawierajacy pakiet TPacketGCItemSet.
     * @param inventoryService Wskaznik na serwis ekwipunku gracza.
     * @return EterBase::PacketResult<void> Zwraca sukces lub blad.
     */
    EterBase::PacketResult<void> DispatchItemSet(std::span<const uint8_t> buffer, UserInterface::Services::IInventoryService* inventoryService)
    {
        if (!inventoryService)
        {
            EterBase::ModernLogger::Error("DispatchItemSet: Otrzymano nullptr dla IInventoryService.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        if (buffer.size() < sizeof(TPacketGCItemSet))
        {
            EterBase::ModernLogger::Error("DispatchItemSet: Zbyt maly rozmiar bufora ({} < {})", buffer.size(), sizeof(TPacketGCItemSet));
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCItemSet*>(buffer.data());

        // Tylko inwentarz i ekwipunek maja sens w kontekscie IInventoryService
        if (packet->pos.window_type != INVENTORY && packet->pos.window_type != EQUIPMENT)
        {
             EterBase::ModernLogger::Debug("DispatchItemSet: Pomijam aktualizacje - window_type to nie INVENTORY/EQUIPMENT (window_type={})", packet->pos.window_type);
             return {};
        }

        UserInterface::Services::InventoryItemView itemView{};
        itemView.slot = EterBase::ItemSlot(packet->pos.cell);
        itemView.vnum = EterBase::ItemVnum(packet->vnum);
        itemView.count = packet->count;
        itemView.isLocked = false;

        std::copy(std::begin(packet->alSockets), std::end(packet->alSockets), std::begin(itemView.sockets));

        for (size_t i = 0; i < std::min<size_t>(7, ITEM_ATTRIBUTE_SLOT_MAX_NUM); ++i)
        {
            itemView.attrTypes[i] = packet->aAttr[i].bType;
            itemView.attrValues[i] = packet->aAttr[i].sValue;
        }

        auto result = inventoryService->SetItem(itemView.slot, itemView);
        if (!result.has_value())
        {
             EterBase::ModernLogger::Error("DispatchItemSet: Blad przy ustawianiu przedmiotu w IInventoryService (vnum={}, slot={})", packet->vnum, packet->pos.cell);
             return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        EterBase::ModernLogger::Info("DispatchItemSet: Pomyslnie odebrano i ustawiono ITEM_SET (vnum={}, slot={}, count={})", packet->vnum, packet->pos.cell, packet->count);
        
        // Zglaszamy event dla UI, wykorzystujac eventbus z InventoryEvents
        UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::InventoryEvents::ItemAcquired(itemView.vnum, itemView.slot, itemView.count));

        return {};
    }
}
