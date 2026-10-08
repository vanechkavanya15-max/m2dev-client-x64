#include "SkillCooltimeHandler.h"
#include "../../Gameplay/SkillDomain.h"
#include "../../../EterBase/LogModern.h"

EterBase::PacketResult<void> SkillCooltimeHandler::HandlePacket(std::span<const uint8_t> buffer, Client::Gameplay::SkillDomain& skillDomain)
{
    if (buffer.size() < sizeof(TPacketGCSkillCoolTimeEnd))
    {
        EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "SkillCooltimeHandler: Buffer underflow. Expected {}, got {}", 
            sizeof(TPacketGCSkillCoolTimeEnd), buffer.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCSkillCoolTimeEnd*>(buffer.data());

    // Update the Domain state
    skillDomain.ResetCooldown(packet->bSkill);

    // Construct strong type for EventBus
    EterBase::SkillId skillId(packet->bSkill);

    // Log the successful handle
    EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "SkillCooltimeHandler: Resetted cooldown for skill {}", skillId.value());

    // Publish event to notify UI subsystem to unlock the icon
    Client::Core::EventBus::GetInstance().Publish(SkillCooltimeEndEvent{skillId});

    return {};
}
