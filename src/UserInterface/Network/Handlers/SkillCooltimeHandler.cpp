#include "StdAfx.h"
#include "SkillCooltimeHandler.h"
#include "../../PythonPlayer.h"
#include "../../../EterBase/LogModern.h"

EterBase::PacketResult<void> SkillCooltimeHandler::HandlePacket(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(SkillCooltimeEndPacket))
    {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const SkillCooltimeEndPacket*>(buffer.data());

    // Wrap the primitive ID into a C++23 strong domain type
    EterBase::SkillId skillId(packet->skillId);

    // Update the C++ memory state directly (decoupled from Python UI)
    CPythonPlayer::Instance().EndSkillCoolTime(skillId.value());

    // Secure C++23 formatting/logging
    EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "Processed skill cooldown for ID {}", skillId.value());

    // Publish the event to notify other subsystems
    UserInterface::Core::EventBus::GetInstance().Publish(SkillCooltimeEndEvent{skillId});

    return {};
}
