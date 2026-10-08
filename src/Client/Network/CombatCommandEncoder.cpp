
#include "CombatCommandEncoder.h"
#include "../../UserInterface/Packet.h"
#include <cstring>

namespace Client::Network {

    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> CombatCommandEncoder::EncodeAttackCommand(
        const Client::Core::AttackCommand& cmd, 
        bool isYmirFraming)
    {
        // ATTACK opcode is defined as `constexpr uint16_t ATTACK = 0x0401;` in `CG` namespace equivalent (or global in Packet.h).
        // Based on Packet.h, it's defined in the global/CG namespace. We will use the direct value or the `CG::ATTACK` if it exists.
        // Wait, the trace showed `constexpr uint16_t ATTACK             = 0x0401;` inside a namespace CG.
        // And TPacketCGAttack is also used.

        TPacketCGAttack packet{};
        packet.bType = cmd.attackType;
        packet.dwVictimVID = cmd.targetVid.Get();
        packet.bCRCMagicCubeProcPiece = 0; // Default or handled by separate system
        packet.bCRCMagicCubeFilePiece = 0; // Default

        if (isYmirFraming)
        {
            // Ymir framing: 1-byte opcode + payload (without length header)
            uint8_t opcode = static_cast<uint8_t>(CG::ATTACK & 0xFF); // or standard fallback
            
            // Layout for 1B Ymir: [1B Header] [bType] [dwVictimVID] [bCRCMagicCubeProcPiece] [bCRCMagicCubeFilePiece]
            // We can serialize manually or just use the struct omitting the length field if the struct matches exactly.
            // A safer and zero-memcpy approach is to serialize the fields sequentially since Ymir struct layout didn't have 2B header and 2B length at the start.
            // Classic Ymir TPacketCGAttack layout:
            // uint8_t header
            // uint8_t bType
            // uint32_t dwVictimVID
            // uint8_t bCRCMagicCubeProcPiece
            // uint8_t bCRCMagicCubeFilePiece
            
            size_t ymirSize = sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t);
            std::vector<uint8_t> buffer(ymirSize, 0);
            size_t offset = 0;

            buffer[offset] = opcode; offset += sizeof(uint8_t);
            buffer[offset] = packet.bType; offset += sizeof(uint8_t);
            std::memcpy(buffer.data() + offset, &packet.dwVictimVID, sizeof(uint32_t)); offset += sizeof(uint32_t);
            buffer[offset] = packet.bCRCMagicCubeProcPiece; offset += sizeof(uint8_t);
            buffer[offset] = packet.bCRCMagicCubeFilePiece; offset += sizeof(uint8_t);

            return buffer;
        }
        else
        {
            // m2dev framing: 2-byte header + 2-byte length + payload
            packet.header = CG::ATTACK;
            packet.length = sizeof(TPacketCGAttack);
            
            std::vector<uint8_t> buffer(sizeof(TPacketCGAttack));
            std::memcpy(buffer.data(), &packet, sizeof(TPacketCGAttack));
            
            return buffer;
        }
    }

    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> CombatCommandEncoder::EncodeShootCommand(
        const Client::Core::ShootCommand& cmd, 
        bool isYmirFraming)
    {
        TPacketCGShoot packet{};
        packet.bType = cmd.skillVnum;

        if (isYmirFraming)
        {
            // Classic Ymir TPacketCGShoot layout:
            // uint8_t header
            // uint8_t bType
            
            uint8_t opcode = static_cast<uint8_t>(CG::SHOOT & 0xFF);
            size_t ymirSize = sizeof(uint8_t) + sizeof(uint8_t);
            std::vector<uint8_t> buffer(ymirSize, 0);
            
            buffer[0] = opcode;
            buffer[1] = packet.bType;
            
            return buffer;
        }
        else
        {
            packet.header = CG::SHOOT;
            packet.length = sizeof(TPacketCGShoot);
            
            std::vector<uint8_t> buffer(sizeof(TPacketCGShoot));
            std::memcpy(buffer.data(), &packet, sizeof(TPacketCGShoot));
            
            return buffer;
        }
    }

} // namespace Client::Network
