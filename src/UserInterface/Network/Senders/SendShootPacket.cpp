#include "StdAfx.h"
#include "../../PythonNetworkStream.h"
#include "../../Packet.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

namespace Network::Senders
{
    EterBase::PacketResult<void> SendShootPacket(CPythonNetworkStream& stream, EterBase::SkillId skillId)
    {
        TPacketCGShoot packet;
        packet.header = CG::SHOOT;
        packet.length = sizeof(packet);
        packet.bType = static_cast<uint8_t>(skillId.value());

        if (!stream.Send(sizeof(packet), &packet))
        {
            EterBase::ModernLogger::Error("Failed to send shoot packet for skill ID {}", skillId.value());
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Debug("Successfully sent shoot packet for skill ID {}", skillId.value());

        return {};
    }

    EterBase::PacketResult<void> SendShootPacket(EterBase::SkillId skillId)
    {
        return SendShootPacket(CPythonNetworkStream::Instance(), skillId);
    }
}
