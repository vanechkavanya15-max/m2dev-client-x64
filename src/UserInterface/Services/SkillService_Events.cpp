#include "../StdAfx.h"
#include "ISkillService.h"
#include "../Core/EventBus.h"
#include "../Network/Handlers/SkillCooltimeHandler.h"
#include "EterBase/LogModern.h"
#include <unordered_map>
#include <vector>

namespace UserInterface::Services
{
    class SkillService final : public ISkillService
    {
    public:
        SkillService() = default;
        ~SkillService() override = default;

        void SetSkillLevel(EterBase::SkillId id, uint8_t level, uint8_t masterType) override
        {
            auto& skill = m_skills[id];
            skill.skillId = id;
            skill.level = level;
            skill.masterType = masterType;
            EterBase::ModernLogger::Debug("SkillService: Set skill level for ID {}, level {}, masterType {}", id.value(), level, masterType);
        }

        void SetSkillCooltime(EterBase::SkillId id, float duration) override
        {
            auto& skill = m_skills[id];
            skill.skillId = id;
            skill.totalCooltime = duration;
            skill.cooltimeRemaining = duration;
            skill.isActived = (duration > 0.0f);
            EterBase::ModernLogger::Debug("SkillService: Set cooltime for ID {} to {}s", id.value(), duration);
        }

        bool IsSkillCooltime(EterBase::SkillId id) const override
        {
            if (auto it = m_skills.find(id); it != m_skills.end())
            {
                return it->second.isActived && it->second.cooltimeRemaining > 0.0f;
            }
            return false;
        }

        float GetSkillCooltimeRemaining(EterBase::SkillId id) const override
        {
            if (auto it = m_skills.find(id); it != m_skills.end())
            {
                return it->second.cooltimeRemaining;
            }
            return 0.0f;
        }

        std::optional<PlayerSkillView> GetSkill(EterBase::SkillId id) const override
        {
            if (auto it = m_skills.find(id); it != m_skills.end())
            {
                return it->second;
            }
            return std::nullopt;
        }

        void UpdateCooltimes(float deltaTime) override
        {
            if (deltaTime <= 0.0f)
                return;

            for (auto& [id, skill] : m_skills)
            {
                if (skill.isActived && skill.cooltimeRemaining > 0.0f)
                {
                    skill.cooltimeRemaining -= deltaTime;

                    if (skill.cooltimeRemaining <= 0.0f)
                    {
                        skill.cooltimeRemaining = 0.0f;
                        skill.isActived = false;

                        EterBase::ModernLogger::Info("SkillService: Cooldown ended for ID {}", id.value());
                        Core::EventBus::GetInstance().Publish(SkillCooltimeEndEvent{id});
                    }
                }
            }
        }

        void Clear() override
        {
            m_skills.clear();
            EterBase::ModernLogger::Debug("SkillService: Cleared all skills");
        }

    private:
        std::unordered_map<EterBase::SkillId, PlayerSkillView> m_skills;
    };
}
