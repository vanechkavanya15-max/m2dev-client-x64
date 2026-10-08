#pragma once

#include <span>
#include <vector>
#include <expected>
#include <cstdint>

#include "../../UserInterface/Packet.h"

namespace EterBase {
    enum class PacketError { BufferUnderflow };

    template <typename T>
    using PacketResult = std::expected<T, PacketError>;
}

namespace Client::Network {

    using TPacketGCGuildSubInfo = TPacketGCGuildInfo;
    using TPacketGCGuildSubWar = TPacketGCGuildWar;

    EterBase::PacketResult<TPacketGCGuild> DecodeGuildHeader(std::span<const uint8_t> buffer);
    EterBase::PacketResult<TPacketGCGuildSubInfo> DecodeGuildSubInfo(std::span<const uint8_t> buffer);
    EterBase::PacketResult<TPacketGCGuildSubMember> DecodeGuildSubMember(std::span<const uint8_t> buffer);
    EterBase::PacketResult<TPacketGCGuildSubWar> DecodeGuildSubWar(std::span<const uint8_t> buffer);

    std::vector<uint8_t> EncodeGuildAddMember(uint32_t vid);
    std::vector<uint8_t> EncodeGuildRemoveMember(uint32_t pid);

}
