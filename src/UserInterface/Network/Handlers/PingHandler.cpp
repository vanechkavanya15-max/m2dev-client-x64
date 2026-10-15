#include "StdAfx.h"
#include "PingHandler.h"

#include <EterBase/Timer.h>

bool PingHandler::HandlePing(INetworkStream& stream, uint32_t& lastPingTime)
{
    PingPacket pingPacket{};

    // Read the ping packet from the stream
    if (!stream.Recv(std::span<uint8_t>(reinterpret_cast<uint8_t*>(&pingPacket), sizeof(pingPacket))))
    {
        return false;
    }

    // Record the time the ping was received locally
    lastPingTime = ELTimer_GetMSec();

    // Synchronize local timer with the server's time
    ELTimer_SetServerMSec(pingPacket.serverTime);

    // Prepare the pong response
    PongPacket pongPacket{};
    pongPacket.header = HEADER_CG_PONG;
    pongPacket.length = sizeof(pongPacket);

    // Send the pong response back to the server
    if (!stream.Send(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&pongPacket), sizeof(pongPacket))))
    {
        return false;
    }

    return true;
}
