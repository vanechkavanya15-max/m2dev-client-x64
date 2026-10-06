#include "../../StdAfx.h"
#include "SkillLevelHandler.h"
#include "../../PythonPlayer.h"
#include <cstring>
#include <cassert>

namespace Network::Handlers
{
    constexpr size_t MAX_SKILL_COUNT = 255;

    SkillUpdateResult SkillLevelHandler::HandleSkillLevel(CPythonPlayer& player, std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(Packets::SkillLevelPacket))
        {
            return { false, false, false };
        }

        const auto* packet = reinterpret_cast<const Packets::SkillLevelPacket*>(buffer.data());
        DWORD slotIndex = 0;

        for (size_t i = 0; i < MAX_SKILL_COUNT; ++i)
        {
            if (player.GetSkillSlotIndex(static_cast<DWORD>(i), &slotIndex))
            {
                player.SetSkillLevel(slotIndex, packet->skillLevels[i]);
            }
        }

        return { true, true, true };
    }

    SkillUpdateResult SkillLevelHandler::HandleSkillLevelNew(CPythonPlayer& player, std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(Packets::SkillLevelNewPacket))
        {
            return { false, false, false };
        }

        const auto* packet = reinterpret_cast<const Packets::SkillLevelNewPacket*>(buffer.data());

        player.SetSkill(7, 0);
        player.SetSkill(8, 0);

        for (size_t i = 0; i < MAX_SKILL_COUNT; ++i)
        {
            const auto& playerSkill = packet->skills[i];

            if (i >= 112 && i <= 115 && playerSkill.level > 0)
            {
                player.SetSkill(7, static_cast<DWORD>(i));
            }

            if (i >= 116 && i <= 119 && playerSkill.level > 0)
            {
                player.SetSkill(8, static_cast<DWORD>(i));
            }

            player.SetSkillLevel_(static_cast<DWORD>(i), playerSkill.masterType, playerSkill.level);
        }

        return { true, true, true };
    }
}
