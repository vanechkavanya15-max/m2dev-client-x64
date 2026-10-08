#include "StdAfx.h"
#include "SendAttackPacket.h"
#include "../../../EterLib/NetStream.h"
#include "../../../EterBase/LogModern.h"
#include <cstring>
#include <optional>
#include <span>

namespace UserInterface::Network::Senders
{

#pragma pack(push, 1)
/**
 * @brief Proxy structure for CG::ATTACK (0x0401) network payload.
 * 
 * Ensures strict 1-byte alignment and modern C++ naming conventions.
 * Maps to legacy TPacketCGAttack memory layout.
 */
struct ProxyPacketCGAttack
{
    uint16_t header;         ///< Packet opcode (CG::ATTACK).
    uint16_t length;         ///< Total length of the packet.
    uint8_t  attackType;     ///< The attack motion or type index.
    uint32_t victimId;       ///< Virtual ID of the target victim.
    uint8_t  crcProcPiece;   ///< CRC piece (legacy/security placeholder).
    uint8_t  crcFilePiece;   ///< CRC piece (legacy/security placeholder).
};
static_assert(sizeof(ProxyPacketCGAttack) == 11, "ProxyPacketCGAttack must be exactly 11 bytes");
#pragma pack(pop)

EterBase::PacketDispatchResult<void> AttackSender::Send(EterBase::EntityId victimId, uint8_t attackType, CNetworkStream* networkStream)
{
    std::optional<CNetworkStream*> streamOpt = networkStream ? std::make_optional(networkStream) : std::nullopt;
    
    return streamOpt.transform([victimId, attackType](CNetworkStream* stream) -> EterBase::PacketDispatchResult<void> {
        ProxyPacketCGAttack attackPacket;
        std::memset(&attackPacket, 0, sizeof(attackPacket));
        
        attackPacket.header = 0x0401; // CG::ATTACK
        attackPacket.length = sizeof(ProxyPacketCGAttack);
        attackPacket.attackType = attackType;
        attackPacket.victimId = victimId.get();
        attackPacket.crcProcPiece = 0;
        attackPacket.crcFilePiece = 0;

        std::span<const uint8_t> packetSpan(reinterpret_cast<const uint8_t*>(&attackPacket), sizeof(attackPacket));
        
        if (!stream->Send(static_cast<int>(packetSpan.size()), packetSpan.data()))
        {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "Failed to send CG::ATTACK for VictimId: {}", victimId.get());
            return EterBase::MakeError(EterBase::PacketDispatchError::QueueFull);
        }

        // Publish event to notify GUI and game systems (Zero-Conflict / Event-Driven)
        UserInterface::Core::EventBus::GetInstance().Publish(NetworkAttackSentEvent(victimId, attackType));

        EterBase::ModernLogger::Log(EterBase::LogLevel::Info, "Successfully queued CG::ATTACK for VictimId: {}", victimId.get());
        
        return {};
    }).value_or(EterBase::MakeError(EterBase::PacketDispatchError::Disconnected));
}

} // namespace UserInterface::Network::Senders
