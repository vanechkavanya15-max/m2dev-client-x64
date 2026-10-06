#include "StdAfx.h"
#include "SendOnClickPacket.h"
#include "../../Packet.h"
#include "../../../EterBase/LogModern.h"
#include "../../PythonNetworkStream.h"

namespace Network::Senders
{
    EterBase::PacketResult<void> SendOnClickPacket(CPythonNetworkStream& stream, EterBase::EntityId targetId)
    {
        TPacketCGOnClick packet{};
        packet.header = CG::ON_CLICK;
        packet.length = sizeof(packet);
        packet.vid    = targetId.value();

        if (!stream.Send(sizeof(packet), &packet))
        {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "Failed to send SendOnClickPacket for targetId: {}", targetId.value());
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Log(EterBase::LogLevel::Trace, "SendOnClickPacket sent successfully for targetId: {}", targetId.value());
        return {};
    }
}
