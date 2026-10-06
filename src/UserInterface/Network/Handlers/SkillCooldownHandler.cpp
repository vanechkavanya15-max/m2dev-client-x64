#include "../../StdAfx.h"
#include "SkillCooldownHandler.h"
#include "../../PythonPlayer.h"

bool SkillCooldownHandler::HandlePacket(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(SkillCoolTimeEndPacket))
    {
        return false;
    }

    const auto* packet = reinterpret_cast<const SkillCoolTimeEndPacket*>(buffer.data());

    // Update the C++ memory state directly (decoupled from UI)
    CPythonPlayer::Instance().EndSkillCoolTime(packet->skillId);

    return true;
}
