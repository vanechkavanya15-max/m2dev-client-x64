#pragma once

#include "ISkillService.h"
#include <unordered_map>

namespace UserInterface::Services
{
    class SkillService : public ISkillService
    {
    public:
        SkillService() = default;
        ~SkillService() override = default;

        void SetSkillLevel(EterBase::SkillId id, uint8_t level, uint8_t masterType) override;
        void SetSkillCooltime(EterBase::SkillId id, float duration) override;
        bool IsSkillCooltime(EterBase::SkillId id) const override;
        float GetSkillCooltimeRemaining(EterBase::SkillId id) const override;
        std::optional<PlayerSkillView> GetSkill(EterBase::SkillId id) const override;
        void UpdateCooltimes(float deltaTime) override;
        void Clear() override;

    private:
        std::unordered_map<EterBase::SkillId, PlayerSkillView> m_skills;
    };
}
