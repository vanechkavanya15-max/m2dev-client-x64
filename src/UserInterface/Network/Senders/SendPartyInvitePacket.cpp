#include "../../StdAfx.h"
#include "SendPartyInvitePacket.h"
#include "../../PythonNetworkStream.h"
#include "../../Packet.h"
#include "../../../EterBase/LogModern.h"

namespace Network::Senders
{
    EterBase::PacketResult<void> SendPartyInvitePacket(EterBase::EntityId targetId)
    {
        TPacketCGPartyInvite packet{};
        packet.header = CG::PARTY_INVITE;
        packet.length = sizeof(packet);
        packet.vid = targetId.get();

        if (!CPythonNetworkStream::Instance().Send(sizeof(packet), &packet))
        {
            EterBase::ModernLogger::Error("SendPartyInvitePacket [{}] - PACKET SEND ERROR", targetId.get());
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Debug(" << SendPartyInvitePacket : {}", targetId.get());
        return {};
    }
}
