#include "PartyPacketCodec.h"
#include <cstring>

namespace Client::Network
{
    PacketResult<TPacketGCPartyInvite> PartyPacketCodec::DecodePartyInvite(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCPartyInvite))
            return std::unexpected(PacketError::BufferUnderflow);

        TPacketGCPartyInvite packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCPartyInvite));
        return packet;
    }

    PacketResult<TPacketGCPartyAdd> PartyPacketCodec::DecodePartyAdd(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCPartyAdd))
            return std::unexpected(PacketError::BufferUnderflow);

        TPacketGCPartyAdd packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCPartyAdd));
        return packet;
    }

    PacketResult<TPacketGCPartyUpdate> PartyPacketCodec::DecodePartyUpdate(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCPartyUpdate))
            return std::unexpected(PacketError::BufferUnderflow);

        TPacketGCPartyUpdate packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCPartyUpdate));
        return packet;
    }

    PacketResult<TPacketGCPartyRemove> PartyPacketCodec::DecodePartyRemove(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCPartyRemove))
            return std::unexpected(PacketError::BufferUnderflow);

        TPacketGCPartyRemove packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCPartyRemove));
        return packet;
    }

    PacketResult<TPacketGCPartyParameter> PartyPacketCodec::DecodePartyParameter(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCPartyParameter))
            return std::unexpected(PacketError::BufferUnderflow);

        TPacketGCPartyParameter packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCPartyParameter));
        return packet;
    }

    std::vector<uint8_t> PartyPacketCodec::EncodePartyInviteAnswer(uint32_t leaderVid, uint8_t accept)
    {
        TPacketCGPartyInviteAnswer packet;
        packet.header = 0x0702; // PARTY_INVITE_ANSWER from packet.h
        packet.length = sizeof(TPacketCGPartyInviteAnswer);
        packet.leader_pid = leaderVid;
        packet.accept = accept;

        std::vector<uint8_t> buffer(sizeof(TPacketCGPartyInviteAnswer));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGPartyInviteAnswer));
        
        return buffer;
    }
}
