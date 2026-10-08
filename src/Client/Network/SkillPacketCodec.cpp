#include "SkillPacketCodec.h"
#include <cstring>
#include <stdexcept>
#include <format>

namespace Client::Network
{
    PacketResult<TPacketGCSkillLevel> SkillPacketCodec::DecodeSkillLevel(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCSkillLevel))
        {
            return std::unexpected(std::format("Buffer too small for TPacketGCSkillLevel ({} < {})", buffer.size(), sizeof(TPacketGCSkillLevel)));
        }

        TPacketGCSkillLevel packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCSkillLevel));
        return packet;
    }

    PacketResult<TPacketGCSkillLevelNew> SkillPacketCodec::DecodeSkillLevelNew(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCSkillLevelNew))
        {
            return std::unexpected(std::format("Buffer too small for TPacketGCSkillLevelNew ({} < {})", buffer.size(), sizeof(TPacketGCSkillLevelNew)));
        }

        // Validate 2554B limit for x64 build
#if defined(_WIN64) || defined(__x86_64__)
        static_assert(sizeof(TPacketGCSkillLevelNew) == 2554, "Invalid x64 packet limit for TPacketGCSkillLevelNew. Expected 2554.");
#endif
        if (sizeof(TPacketGCSkillLevelNew) != 2554)
        {
            return std::unexpected(std::format("Invalid packet limit for TPacketGCSkillLevelNew (expected 2554, got {})", sizeof(TPacketGCSkillLevelNew)));
        }

        TPacketGCSkillLevelNew packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCSkillLevelNew));
        return packet;
    }

    PacketResult<TPacketGCSkillCoolTimeEnd> SkillPacketCodec::DecodeSkillCooltimeEnd(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCSkillCoolTimeEnd))
        {
            return std::unexpected(std::format("Buffer too small for TPacketGCSkillCoolTimeEnd ({} < {})", buffer.size(), sizeof(TPacketGCSkillCoolTimeEnd)));
        }

        TPacketGCSkillCoolTimeEnd packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCSkillCoolTimeEnd));
        return packet;
    }

    std::vector<uint8_t> SkillPacketCodec::EncodeUseSkill(uint32_t skillVnum, uint32_t targetVid)
    {
        std::vector<uint8_t> buffer(sizeof(TPacketCGUseSkill));
        TPacketCGUseSkill* packet = reinterpret_cast<TPacketCGUseSkill*>(buffer.data());
        
        packet->header = CG::USE_SKILL;
        packet->length = sizeof(TPacketCGUseSkill);
        packet->dwVnum = skillVnum;
        packet->dwTargetVID = targetVid;
        
        return buffer;
    }
}
