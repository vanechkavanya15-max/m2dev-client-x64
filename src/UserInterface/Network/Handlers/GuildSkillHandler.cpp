#include "StdAfx.h"
#include "GuildSkillHandler.h"
#include "../../PythonGuild.h"
#include <algorithm>
#include <cstring>

/**
 * @brief Parses and applies the skill info payload to the internal memory state.
 *
 * It decouples the state update from the legacy UI refresh method, returning 
 * success if the packet is of the expected length and correctly copied into memory.
 * 
 * @param payload Binary buffer containing the incoming data for the packet.
 * @return true If the payload size is valid and data was processed successfully.
 * @return false If the payload size mismatches the expected structure size.
 */
bool GuildSkillHandler::HandleSkillInfo(std::span<const uint8_t> payload)
{
    if (payload.size() != sizeof(GuildSkillInfoPacket))
    {
        TraceError("GuildSkillHandler::HandleSkillInfo - Invalid payload size. Expected %zu, got %zu.",
                   sizeof(GuildSkillInfoPacket), payload.size());
        return false;
    }

    const auto* packet = reinterpret_cast<const GuildSkillInfoPacket*>(payload.data());
    auto& rSkillData = CPythonGuild::Instance().GetGuildSkillDataRef();

    rSkillData.bySkillPoint = packet->skillPoint;
    std::memcpy(rSkillData.bySkillLevel, packet->skillLevel, sizeof(packet->skillLevel));
    rSkillData.wGuildPoint = packet->guildPoint;
    rSkillData.wMaxGuildPoint = packet->maxGuildPoint;

    Tracef(" <SkillInfo> %d / %d, %d\n", rSkillData.bySkillPoint, rSkillData.wGuildPoint, rSkillData.wMaxGuildPoint);

    // Completely decoupled from GUI. No calls to __RefreshGuildWindowSkillPage()
    return true;
}
