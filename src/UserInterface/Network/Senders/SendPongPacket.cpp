#include "../../StdAfx.h"
#include "SendPongPacket.h"

#include "../../../EterLib/NetStream.h"
#include "../../../EterLib/ControlPackets.h"
#include "../../../EterBase/LogModern.h"
#include <cstring>

namespace Network::Senders {

EterBase::PacketResult<void> PongSender::Send(CNetworkStream* networkStream) {
    if (!networkStream) {
        EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "PongSender::Send failed: networkStream is null.");
        return std::unexpected(EterBase::PacketError::SessionClosed);
    }

    TPacketCGPong pongPacket;
    std::memset(&pongPacket, 0, sizeof(pongPacket));
    pongPacket.header = CG::PONG;
    pongPacket.length = sizeof(pongPacket);

    if (!networkStream->Send(sizeof(pongPacket), &pongPacket)) {
        EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "PongSender::Send failed: could not send packet via networkStream.");
        return std::unexpected(EterBase::PacketError::SessionClosed);
    }

    EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "PongSender::Send: PONG packet sent successfully.");
    return {}; // Success
}

} // namespace Network::Senders
