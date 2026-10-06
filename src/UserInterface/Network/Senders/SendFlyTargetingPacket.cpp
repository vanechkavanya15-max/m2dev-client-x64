#include "../../StdAfx.h"
#include "SendFlyTargetingPacket.h"
#include "../../Packet.h"
#include "../../PythonNetworkStream.h"
#include "../../PythonBackground.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace UserInterface::Network::Senders {

EterBase::PacketResult<void> SendFlyTargetingPacket(
    CPythonNetworkStream& stream, 
    EterBase::EntityId targetId, 
    int32_t x, 
    int32_t y
) {
    if (targetId.value() == 0) {
        EterBase::ModernLogger::Warn("SendFlyTargetingPacket: Invalid Target ID: {}", targetId.value());
    }

    TPacketCGFlyTargeting packet{};
    packet.header = CG::FLY_TARGETING;
    packet.length = sizeof(packet);
    packet.dwTargetVID = targetId.value();
    packet.lX = x;
    packet.lY = y;

    CPythonBackground::Instance().LocalPositionToGlobalPosition(packet.lX, packet.lY);

    if (!stream.Send(sizeof(packet), &packet)) {
        EterBase::ModernLogger::Error("SendFlyTargetingPacket: stream.Send failed for Target VID {}.", packet.dwTargetVID);
        return EterBase::MakeError(EterBase::PacketError::SessionClosed);
    }

    EterBase::ModernLogger::Debug("SendFlyTargetingPacket: Successfully sent for Target VID {} at ({}, {})", packet.dwTargetVID, packet.lX, packet.lY);
    return {};
}

} // namespace UserInterface::Network::Senders
