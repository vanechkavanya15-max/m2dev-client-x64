#include "StandardX64ProtocolDriver.h"
#include "../Protocol.h"
#include "UserInterface/Network/Routers/PhaseGamePacketDispatcher.h"
#include <cstring>
#include <cmath>

namespace Network::Protocol::Drivers
{
    std::optional<FrameHeaderInfo> StandardX64ProtocolDriver::InspectFrame(
        std::span<const uint8_t> buffer) const noexcept
    {
        // Wymagane minimum 4 bajty: [header:2][length:2]
        if (buffer.size() < 4)
        {
            return std::nullopt;
        }

        uint16_t header = static_cast<uint16_t>(buffer[0] | (static_cast<uint16_t>(buffer[1]) << 8));
        uint32_t length = static_cast<uint32_t>(buffer[2] | (static_cast<uint32_t>(buffer[3]) << 8));

        // Dlugosc musi zawierac sie w przedziale [4, 65000]
        if (length < 4 || length > 65000)
        {
            return std::nullopt;
        }

        return FrameHeaderInfo{
            .unifiedOpcode = header,
            .packetLength = length,
            .headerSize = 4
        };
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeAttack(
        const Domain::AttackCommand& cmd) const
    {
        TPacketCGAttack packet{};
        packet.header = 0x0401; // Standardowy naglowek CG::ATTACK x64
        packet.length = sizeof(TPacketCGAttack);
        packet.bType = cmd.attackType;
        packet.dwVictimVID = cmd.targetVid;
        packet.bCRCMagicCubeProcPiece = 0;
        packet.bCRCMagicCubeFilePiece = 0;

        std::vector<uint8_t> buffer(sizeof(TPacketCGAttack));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGAttack));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeMove(
        const Domain::MoveCommand& cmd) const
    {
        TPacketCGMove packet{};
        packet.header = 0x0301; // Standardowy naglowek CG::MOVE x64
        packet.length = sizeof(TPacketCGMove);
        packet.bFunc = cmd.func;
        packet.bArg = static_cast<uint8_t>(cmd.arg);

        float rot = std::fmod(cmd.rotationDegrees, 360.0f);
        if (rot < 0.0f)
        {
            rot += 360.0f;
        }
        packet.bRot = static_cast<uint8_t>(rot / 5.0f); // Segmenty rotacji w stopniach (0..72)

        packet.lX = cmd.x;
        packet.lY = cmd.y;
        packet.dwTime = cmd.time;

        std::vector<uint8_t> buffer(sizeof(TPacketCGMove));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGMove));
        return buffer;
    }

    bool StandardX64ProtocolDriver::DispatchInbound(
        uint16_t unifiedOpcode,
        std::span<const uint8_t> payload,
        UserInterface::Contracts::IGameEventSink* /*pSink*/)
    {
        if (payload.empty())
        {
            return false;
        }

        // Delegacja do PhaseGamePacketDispatcher dla architektury x64
        return UserInterface::Network::Routers::PhaseGamePacketDispatcher::Instance().DispatchPacket(
            unifiedOpcode, payload.data(), payload.size());
    }
}
