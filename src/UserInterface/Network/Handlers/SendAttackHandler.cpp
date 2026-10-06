#include "StdAfx.h"
#include "SendAttackHandler.h"
#include "../../../EterLib/NetStream.h"
#include <cstring> // For std::memset

// Define proxy structures explicitly packed, as requested, to avoid touching other files.
#pragma pack(push, 1)
/**
 * @brief Proxy structure representing the primary attack packet sent to the server.
 * 
 * Ensures strict 1-byte alignment to match network protocol specifications.
 */
struct ProxyPacketCGAttack
{
    uint8_t  header;        ///< Packet header identifier.
    uint8_t  attackType;    ///< Type of attack (e.g., standard weapon).
    uint32_t targetVid;     ///< Virtual ID of the target victim.
    uint16_t sequence;      ///< Synchronization sequence/CRC counter.
};
static_assert(sizeof(ProxyPacketCGAttack) == 8, "ProxyPacketCGAttack must be 8 bytes");

/**
 * @brief Proxy structure representing the alternative attack packet (e.g., specific motions/combo).
 * 
 * Ensures strict 1-byte alignment to match network protocol specifications.
 */
struct ProxyPacketCGAttackAlt
{
    uint8_t  header;        ///< Packet header identifier.
    uint32_t targetVid;     ///< Virtual ID of the target victim.
    uint32_t attackMotion;  ///< Specific motion/combo index for the attack.
};
static_assert(sizeof(ProxyPacketCGAttackAlt) == 9, "ProxyPacketCGAttackAlt must be 9 bytes");
#pragma pack(pop)

bool SendAttackHandler::SendAttack(uint32_t targetId, uint32_t attackMotion, uint16_t sequence, CNetworkStream* networkStream)
{
    if (!networkStream)
        return false;

    ProxyPacketCGAttack attackPacket;
    std::memset(&attackPacket, 0, sizeof(attackPacket));
    attackPacket.header = Beavium::CG::ATTACK;
    attackPacket.attackType = 0; // standard weapon attack
    attackPacket.targetVid = targetId;
    attackPacket.sequence = sequence;

    std::span<const uint8_t> attackBuffer(reinterpret_cast<const uint8_t*>(&attackPacket), sizeof(attackPacket));
    if (!networkStream->Send(static_cast<int>(attackBuffer.size()), attackBuffer.data()))
    {
        return false;
    }

    if (attackMotion > 0)
    {
        ProxyPacketCGAttackAlt attackAltPacket;
        std::memset(&attackAltPacket, 0, sizeof(attackAltPacket));
        attackAltPacket.header = Beavium::CG::ATTACK_ALT;
        attackAltPacket.targetVid = targetId;
        attackAltPacket.attackMotion = attackMotion;

        std::span<const uint8_t> altBuffer(reinterpret_cast<const uint8_t*>(&attackAltPacket), sizeof(attackAltPacket));
        if (!networkStream->Send(static_cast<int>(altBuffer.size()), altBuffer.data()))
        {
            return false;
        }
    }

    return true;
}
