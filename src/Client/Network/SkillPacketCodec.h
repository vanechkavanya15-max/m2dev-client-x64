#pragma once

#include <span>
#include <cstdint>
#include <vector>
#include <expected>

#include "../../UserInterface/Packet.h"

namespace Client::Network
{
    template <typename T>
    using PacketResult = std::expected<T, std::string>;
    class SkillPacketCodec
    {
    public:
        static PacketResult<TPacketGCSkillLevel> DecodeSkillLevel(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCSkillLevelNew> DecodeSkillLevelNew(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCSkillCoolTimeEnd> DecodeSkillCooltimeEnd(std::span<const uint8_t> buffer);
        static std::vector<uint8_t> EncodeUseSkill(uint32_t skillVnum, uint32_t targetVid);
    };
}
