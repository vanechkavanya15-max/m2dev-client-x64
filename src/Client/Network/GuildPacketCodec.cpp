#include "GuildPacketCodec.h"
#include <cstring>

namespace Client::Network {

    EterBase::PacketResult<TPacketGCGuild> DecodeGuildHeader(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCGuild)) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }
        TPacketGCGuild packet{};
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCGuild));
        return packet;
    }

    EterBase::PacketResult<TPacketGCGuildSubInfo> DecodeGuildSubInfo(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCGuildSubInfo)) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }
        TPacketGCGuildSubInfo packet{};
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCGuildSubInfo));
        return packet;
    }

    EterBase::PacketResult<TPacketGCGuildSubMember> DecodeGuildSubMember(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCGuildSubMember)) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }
        TPacketGCGuildSubMember packet{};
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCGuildSubMember));
        return packet;
    }

    EterBase::PacketResult<TPacketGCGuildSubWar> DecodeGuildSubWar(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCGuildSubWar)) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }
        TPacketGCGuildSubWar packet{};
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCGuildSubWar));
        return packet;
    }

    std::vector<uint8_t> EncodeGuildAddMember(uint32_t vid) {
        std::vector<uint8_t> buffer(sizeof(TPacketCGGuild) + sizeof(uint32_t));
        
        TPacketCGGuild packet{};
        packet.header = CG::GUILD;
        packet.length = static_cast<uint16_t>(buffer.size());
        packet.bySubHeader = GuildSub::CG::ADD_MEMBER;
        
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGGuild));
        std::memcpy(buffer.data() + sizeof(TPacketCGGuild), &vid, sizeof(uint32_t));
        
        return buffer;
    }

    std::vector<uint8_t> EncodeGuildRemoveMember(uint32_t pid) {
        std::vector<uint8_t> buffer(sizeof(TPacketCGGuild) + sizeof(uint32_t));
        
        TPacketCGGuild packet{};
        packet.header = CG::GUILD;
        packet.length = static_cast<uint16_t>(buffer.size());
        packet.bySubHeader = GuildSub::CG::REMOVE_MEMBER;
        
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGGuild));
        std::memcpy(buffer.data() + sizeof(TPacketCGGuild), &pid, sizeof(uint32_t));
        
        return buffer;
    }

}
