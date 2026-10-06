#include "../../StdAfx.h"
#include "ViewEquipHandler.h"
#include "../../../EterBase/LogModern.h"

namespace Network::Handlers
{
    /**
     * @brief Przetwarza pakiet podglądu ekwipunku gracza zwracając PacketResult C++23.
     * @param buffer Bufor bajtów pakietu.
     * @return EterBase::PacketResult<void> ze statusem sukcesu lub błędu.
     */
    EterBase::PacketResult<void> ProcessViewEquip(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCViewEquip))
        {
            EterBase::ModernLogger::Error("ViewEquipHandler: Buffer underflow. Expected >= {} bytes, got {}", sizeof(TPacketGCViewEquip), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCViewEquip*>(buffer.data());
        
        const EterBase::EntityId targetId(packet->dwVID);
        
        std::span<const TEquipmentItemSet> equips_span(packet->equips, WEAR_MAX_NUM);
        
        // Emit event to update the UI decoupling the logic
        UserInterface::Core::EventBus::GetInstance().Publish(ViewEquipEvent(targetId, equips_span));

        EterBase::ModernLogger::Debug("ViewEquipHandler: Successfully processed ViewEquip packet for VID: {}", targetId.value());

        return {};
    }
}
