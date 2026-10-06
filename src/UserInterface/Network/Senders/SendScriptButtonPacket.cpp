#include "StdAfx.h"
#include "SendScriptButtonPacket.h"
#include "../../Packet.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterLib/NetStream.h"

#include <cstring>
#include <span>

namespace Network::Senders {

    EterBase::PacketResult<void> SendScriptButtonPacket(uint32_t buttonIndex, CNetworkStream* networkStream)
    {
        if (!networkStream)
        {
            EterBase::ModernLogger::Error("SendScriptButtonPacket failed: networkStream is null.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        TPacketCGScriptButton packet;
        std::memset(&packet, 0, sizeof(packet));

        packet.header = CG::SCRIPT_BUTTON;
        packet.length = static_cast<uint16_t>(sizeof(TPacketCGScriptButton));
        packet.idx = buttonIndex;

        std::span<const uint8_t> buffer(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));

        if (!networkStream->Send(static_cast<int>(buffer.size()), buffer.data()))
        {
            EterBase::ModernLogger::Error("SendScriptButtonPacket failed: network send error.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Debug("Successfully sent SCRIPT_BUTTON packet for buttonIndex={}.", buttonIndex);

        UserInterface::Core::EventBus::GetInstance().Publish(
            UserInterface::Core::ScriptButtonSentEvent(buttonIndex)
        );

        return {}; // Success (std::expected<void>)
    }

} // namespace Network::Senders
