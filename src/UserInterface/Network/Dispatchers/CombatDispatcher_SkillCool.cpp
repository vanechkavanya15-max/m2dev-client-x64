#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../PythonPlayer.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"
#include "../Handlers/SkillCooltimeHandler.h"

#include <expected>
#include <span>

namespace UserInterface::Network::Dispatchers {

/**
 * @brief Handles the SKILL_COOLTIME_END packet from the server.
 * 
 * Extracts the skill ID from the packet, passes it to the UI (PythonPlayer), 
 * and triggers an event on the EventBus for decoupled systems.
 * 
 * @param buffer The binary span representing the packet payload.
 * @return EterBase::PacketResult<void> Success or packet-related error.
 */
EterBase::PacketResult<void> DispatchSkillCooltime(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(TPacketGCSkillCoolTimeEnd))
    {
        EterBase::ModernLogger::Log(EterBase::LogLevel::Error, 
            "CombatDispatcher: SKILL_COOLTIME_END packet buffer underflow. Expected: {}, Got: {}",
            sizeof(TPacketGCSkillCoolTimeEnd), buffer.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCSkillCoolTimeEnd*>(buffer.data());

    EterBase::SkillId skillId(packet->bSkill);

    CPythonPlayer::Instance().EndSkillCoolTime(skillId.value());
    EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "CombatDispatcher: EndSkillCoolTime processed for skill {}", skillId.value());
    
    EterBase::ModernLogger::Log(EterBase::LogLevel::Info, "DispatchSkillCooltime successfully processed skill {}", skillId.value());

    UserInterface::Core::EventBus::Instance().Publish(SkillCooltimeEndEvent{skillId});
    
    return {};
}

} // namespace UserInterface::Network::Dispatchers
