#include "../../StdAfx.h"
/**
 * @file SendExchangeStartPacket.cpp
 * @brief Implementation of the SendExchangeStartPacketHandler.
 */

#include "SendExchangeStartPacket.h"
#include "../../Packet.h"
#include "../../../EterLib/NetStream.h"
#include "../../../EterBase/LogModern.h"
#include <cstring>
#include <span>

// Define proxy structures explicitly packed, as requested, to avoid touching other files.
#pragma pack(push, 1)

/**
 * @brief Proxy structure representing the exchange start packet sent to the server.
 * 
 * Ensures strict 1-byte alignment to match network protocol specifications.
 * This directly matches TPacketCGExchange but is scoped locally to prevent conflicts.
 */
struct ProxyPacketCGExchangeStart
{
    uint16_t header;       ///< Packet header identifier (CG::EXCHANGE)
    uint16_t length;       ///< Total length of the packet
    uint8_t  subheader;    ///< Specific exchange action (ExchangeSub::CG::START)
    uint32_t targetVid;    ///< Primary argument (Virtual ID of the target)
    uint8_t  padding1;     ///< Secondary argument padding (arg2)
    TItemPos paddingPos;   ///< ItemPos padding (Pos)
};
static_assert(sizeof(ProxyPacketCGExchangeStart) == 13, "ProxyPacketCGExchangeStart must be 13 bytes to match TPacketCGExchange");

#pragma pack(pop)

EterBase::PacketResult<void> SendExchangeStartPacketHandler::SendExchangeStart(EterBase::EntityId targetId, CNetworkStream* networkStream)
{
    if (!networkStream)
    {
        EterBase::ModernLogger::Error("SendExchangeStartPacketHandler::SendExchangeStart: networkStream is null.");
        return std::unexpected(EterBase::PacketError::SessionClosed);
    }

    if (!targetId)
    {
        EterBase::ModernLogger::Error("SendExchangeStartPacketHandler::SendExchangeStart: invalid targetId.");
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    ProxyPacketCGExchangeStart packet;
    std::memset(&packet, 0, sizeof(packet));

    packet.header = CG::EXCHANGE;
    packet.length = static_cast<uint16_t>(sizeof(packet));
    packet.subheader = ExchangeSub::CG::START;
    packet.targetVid = targetId.get();

    std::span<const uint8_t> buffer(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));

    if (!networkStream->Send(static_cast<int>(buffer.size()), buffer.data()))
    {
        EterBase::ModernLogger::Error("SendExchangeStartPacketHandler::SendExchangeStart: failed to send packet for target {}", targetId.get());
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    EterBase::ModernLogger::Debug("SendExchangeStartPacketHandler::SendExchangeStart: successfully sent for target {}", targetId.get());

    return {}; // Success
}
