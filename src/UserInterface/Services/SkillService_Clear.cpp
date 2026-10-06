#include "../StdAfx.h"
#include "ISkillService.h"
#include "../Domain/SkillRegistry.h"
#include "../Domain/SkillCooldownMapModel.h"
#include "../../EterBase/LogModern.h"

namespace UserInterface::Services
{
    class SkillService final : public ISkillService
    {
    public:
        void SetSkillLevel(EterBase::SkillId id, uint8_t level, uint8_t masterType) override;
        void SetSkillCooltime(EterBase::SkillId id, float duration) override;
        bool IsSkillCooltime(EterBase::SkillId id) const override;
        float GetSkillCooltimeRemaining(EterBase::SkillId id) const override;
        std::optional<PlayerSkillView> GetSkill(EterBase::SkillId id) const override;
        void UpdateCooltimes(float deltaTime) override;

        void Clear() override;

    private:
        Metin2::Domain::SkillRegistry m_registry;
        UserInterface::Domain::SkillCooldownMapModel m_cooldowns;
    };

    void SkillService::Clear()
    {
        EterBase::ModernLogger::Info("SkillService::Clear - Resetting skill states and cooldowns.");
        
        m_registry.Clear();
        m_cooldowns.ClearAll();

        // Note: Event publishing is handled automatically by SkillCooldownMapModel::ClearAll(),
        // which completely fulfills the decouple requirement.
    }
}
