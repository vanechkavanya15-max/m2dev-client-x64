#include "CombatPacketCodec.h"
#include <cstring>

namespace Network::CombatPacketCodec
{
    // CInstanceBase::DamageFlag values
    constexpr uint8_t DAMAGE_NORMAL    = (1 << 0);
    constexpr uint8_t DAMAGE_POISON    = (1 << 1);
    constexpr uint8_t DAMAGE_DODGE     = (1 << 2);
    constexpr uint8_t DAMAGE_BLOCK     = (1 << 3);
    constexpr uint8_t DAMAGE_PENETRATE = (1 << 4);
    constexpr uint8_t DAMAGE_CRITICAL  = (1 << 5);

    constexpr uint8_t VALID_DAMAGE_FLAGS = 
        DAMAGE_NORMAL | DAMAGE_POISON | DAMAGE_DODGE | 
        DAMAGE_BLOCK | DAMAGE_PENETRATE | DAMAGE_CRITICAL;

    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeAttack(const TPacketCGAttack& packet)
    {
        std::vector<uint8_t> buffer(sizeof(TPacketCGAttack));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGAttack));
        return buffer;
    }

    [[nodiscard]] EterBase::PacketResult<TPacketCGAttack> DecodeAttack(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketCGAttack))
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);

        TPacketCGAttack packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketCGAttack));
        return packet;
    }

    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeShoot(const TPacketCGShoot& packet)
    {
        std::vector<uint8_t> buffer(sizeof(TPacketCGShoot));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGShoot));
        return buffer;
    }

    [[nodiscard]] EterBase::PacketResult<TPacketCGShoot> DecodeShoot(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketCGShoot))
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);

        TPacketCGShoot packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketCGShoot));
        return packet;
    }

    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeDamageInfo(const TPacketGCDamageInfo& packet)
    {
        // For encoding, we assume the caller provides valid damage flags, but we can clamp them.
        TPacketGCDamageInfo safePacket = packet;
        safePacket.flag &= VALID_DAMAGE_FLAGS;

        std::vector<uint8_t> buffer(sizeof(TPacketGCDamageInfo));
        std::memcpy(buffer.data(), &safePacket, sizeof(TPacketGCDamageInfo));
        return buffer;
    }

    [[nodiscard]] EterBase::PacketResult<TPacketGCDamageInfo> DecodeDamageInfo(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCDamageInfo))
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);

        TPacketGCDamageInfo packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCDamageInfo));

        // Validate damage flag safely
        if ((packet.flag & ~VALID_DAMAGE_FLAGS) != 0)
        {
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        return packet;
    }

    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeDead(const TPacketGCDead& packet)
    {
        std::vector<uint8_t> buffer(sizeof(TPacketGCDead));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketGCDead));
        return buffer;
    }

    [[nodiscard]] EterBase::PacketResult<TPacketGCDead> DecodeDead(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCDead))
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);

        TPacketGCDead packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCDead));
        return packet;
    }
}
