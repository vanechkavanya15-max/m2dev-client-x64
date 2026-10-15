#pragma once

#include "ISkillService.h"
#include <unordered_map>
#include <optional>
#include <algorithm>

namespace UserInterface::Services
{
    class SkillService : public ISkillService
    {
    public:
        static SkillService& Instance()
        {
            static SkillService s_instance;
            return s_instance;
        }

        SkillService() = default;
        ~SkillService() override = default;

        void SetSkillLevel(EterBase::SkillId id, uint8_t level, uint8_t masterType) override;
        void SetSkillCooltime(EterBase::SkillId id, float duration) override;

        bool IsSkillCooltime(EterBase::SkillId id) const override
        {
            auto it = m_skills.find(id);
            if (it != m_skills.end())
            {
                return it->second.cooltimeRemaining > 0.0f;
            }
            return false;
        }

        float GetSkillCooltimeRemaining(EterBase::SkillId id) const override
        {
            auto it = m_skills.find(id);
            if (it != m_skills.end())
            {
                return it->second.cooltimeRemaining;
            }
            return 0.0f;
        }

        std::optional<PlayerSkillView> GetSkill(EterBase::SkillId id) const override
        {
            auto it = m_skills.find(id);
            if (it != m_skills.end())
            {
                return it->second;
            }
            return std::nullopt;
        }

        void UpdateCooltimes(float deltaTime) override
        {
            for (auto& [_, s] : m_skills)
            {
                if (s.cooltimeRemaining > 0.0f)
                {
                    s.cooltimeRemaining = std::max(0.0f, s.cooltimeRemaining - deltaTime);
                }
            }
        }

        void Clear() override;

    private:
        std::unordered_map<EterBase::SkillId, PlayerSkillView> m_skills;
    };
}
