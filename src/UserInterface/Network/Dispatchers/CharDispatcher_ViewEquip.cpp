#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

#include <span>
#include <cstdint>

namespace Network::Dispatchers
{
    namespace
    {
        /**
         * @brief Zdarzenie emitowane po otrzymaniu pakietu podglądu ekwipunku gracza.
         */
        struct ViewEquipEvent : public UserInterface::Core::IEvent
        {
            EterBase::EntityId targetVid;
            std::span<const TEquipmentItemSet> equips;

            /**
             * @brief Konstruktor zdarzenia podglądu ekwipunku.
             * @param vid Identyfikator gracza.
             * @param eq Zakres wyekwipowanych przedmiotów.
             */
            ViewEquipEvent(EterBase::EntityId vid, std::span<const TEquipmentItemSet> eq)
                : targetVid(vid), equips(eq)
            {
            }
        };
    }

    /**
     * @brief Przetwarza pakiet podglądu ekwipunku gracza.
     * @param buffer Bufor bajtów pakietu do przetworzenia.
     * @return EterBase::PacketResult<void> ze statusem parsowania pakietu.
     */
    EterBase::PacketResult<void> DispatchViewEquip(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCViewEquip))
        {
            EterBase::ModernLogger::Error("CharDispatcher_ViewEquip: Buffer underflow. Expected >= {} bytes, got {}", 
                                          sizeof(TPacketGCViewEquip), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCViewEquip*>(buffer.data());

        const EterBase::EntityId targetId(packet->dwVID);
        
        std::span<const TEquipmentItemSet> equips_span(packet->equips, WEAR_MAX_NUM);
        
        EterBase::ModernLogger::Debug("CharDispatcher_ViewEquip: Successfully processed ViewEquip packet for VID: {}", targetId.value());

        // Emit event to update the UI decoupling the logic
        UserInterface::Core::EventBus::GetInstance().Publish(ViewEquipEvent(targetId, equips_span));

        return {};
    }
}
