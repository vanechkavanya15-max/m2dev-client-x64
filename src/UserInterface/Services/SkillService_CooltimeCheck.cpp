#include "../StdAfx.h"
#include "ISkillService.h"
#include "../Packet.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace UserInterface::Services
{
    /**
     * @brief Event emitted when a skill is requested but is on cooldown.
     */
    struct SkillOnCooldownEvent : public UserInterface::Core::IEvent
    {
        EterBase::SkillId skillId{0};
        float remainingTime{0.0f};

        SkillOnCooldownEvent(EterBase::SkillId id, float time) : skillId(id), remainingTime(time) {}
    };

    /**
     * @brief Event emitted when a skill is ready to be used.
     */
    struct SkillReadyToUseEvent : public UserInterface::Core::IEvent
    {
        EterBase::SkillId skillId{0};

        explicit SkillReadyToUseEvent(EterBase::SkillId id) : skillId(id) {}
    };

    /**
     * @brief Microservice class for skill cooltime checking operations.
     */
    class SkillService_CooltimeCheck
    {
    public:
        /**
         * @brief Checks whether the specified skill is ready to use, emitting events via EventBus.
         * 
         * @param skillService A reference to the ISkillService to query skill data.
         * @param skillId The ID of the skill to check.
         * @return std::expected<void, EterBase::CombatError> Success or error reason.
         */
        static std::expected<void, EterBase::CombatError> CheckSkillReady(const ISkillService& skillService, EterBase::SkillId skillId)
        {
            if (skillId.value() >= SKILL_MAX_NUM)
            {
                EterBase::ModernLogger::Warning("CheckSkillReady: Skill ID {} is out of bounds.", skillId.value());
                return std::unexpected(EterBase::CombatError::InvalidAction);
            }

            auto skillOpt = skillService.GetSkill(skillId);
            if (!skillOpt.has_value())
            {
                EterBase::ModernLogger::Warning("CheckSkillReady: Skill ID {} not found in skill service.", skillId.value());
                return std::unexpected(EterBase::CombatError::InvalidAction);
            }

            if (skillService.IsSkillCooltime(skillId))
            {
                float remaining = skillService.GetSkillCooltimeRemaining(skillId);
                EterBase::ModernLogger::Debug("CheckSkillReady: Skill ID {} is on cooldown for {}s.", skillId.value(), remaining);
                
                UserInterface::Core::EventBus::GetInstance().Publish(SkillOnCooldownEvent{skillId, remaining});
                
                return std::unexpected(EterBase::CombatError::OnCooldown);
            }

            EterBase::ModernLogger::Info("CheckSkillReady: Skill ID {} is ready to use.", skillId.value());
            UserInterface::Core::EventBus::GetInstance().Publish(SkillReadyToUseEvent{skillId});

            return {};
        }
    };
}
