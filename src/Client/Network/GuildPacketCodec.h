#pragma once

#include <span>
#include <vector>
#include <expected>
#include <cstdint>

#include "EterBase/Result.h"
#include "../../UserInterface/Packet.h"

namespace Client::Network {

    using TPacketGCGuildSubInfo = TPacketGCGuildInfo;
    using TPacketGCGuildSubWar = TPacketGCGuildWar;

    EterBase::PacketResult<TPacketGCGuild> DecodeGuildHeader(std::span<const uint8_t> buffer);
    EterBase::PacketResult<TPacketGCGuildSubInfo> DecodeGuildSubInfo(std::span<const uint8_t> buffer);
    EterBase::PacketResult<TPacketGCGuildSubMember> DecodeGuildSubMember(std::span<const uint8_t> buffer);
    EterBase::PacketResult<TPacketGCGuildSubWar> DecodeGuildSubWar(std::span<const uint8_t> buffer);

    std::vector<uint8_t> EncodeGuildAddMember(uint32_t vid);
    std::vector<uint8_t> EncodeGuildRemoveMember(uint32_t pid);

    class GuildPacketCodec {
    public:
        static EterBase::PacketResult<TPacketGCGuild> Decode(std::span<const uint8_t> buffer) {
            return DecodeGuildHeader(buffer);
        }
    };

}
