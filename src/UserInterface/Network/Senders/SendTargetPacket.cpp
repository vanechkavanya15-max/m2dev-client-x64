#include "../../StdAfx.h"
#include "SendTargetPacket.h"
#include "../../Packet.h"
#include <EterLib/NetStream.h>
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace UserInterface::Network::Senders {

EterBase::PacketResult<void> SendTargetPacket::Execute(CNetworkStream& stream, EterBase::EntityId targetId)
{
    TPacketCGTarget packet{};
    packet.header = CG::TARGET;
    packet.length = sizeof(packet);
    packet.dwVID = targetId.value();

    if (!stream.Send(sizeof(packet), &packet))
    {
        EterBase::ModernLogger::Error("Failed to send CG::TARGET packet. EntityId: {}", targetId.value());
        return EterBase::MakeError(EterBase::PacketError::SessionClosed);
    }

    EterBase::ModernLogger::Debug("Sent CG::TARGET packet successfully. EntityId: {}", targetId.value());
    
    // Publish TargetBoardRefreshEvent through EventBus to decouple from GUI
    UserInterface::Core::EventBus::GetInstance().Publish(
        UserInterface::Core::TargetBoardRefreshEvent(targetId.value())
    );

    return {};
}

} // namespace UserInterface::Network::Senders
