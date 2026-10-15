#include "../StdAfx.h"
#include "SkillService.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"

namespace UserInterface::Services
{
    struct SkillLevelChangedEvent : public Core::IEvent
    {
        EterBase::SkillId skillId;
        uint8_t level;
        uint8_t masterType;
        
        SkillLevelChangedEvent(EterBase::SkillId id, uint8_t lvl, uint8_t mType)
            : skillId(id), level(lvl), masterType(mType) {}
    };

    void SkillService::SetSkillLevel(EterBase::SkillId id, uint8_t level, uint8_t masterType)
    {
        if (masterType > 3 || level > 40)
        {
            EterBase::ModernLogger::Error("SkillService::SetSkillLevel blad walidacji dla SkillId [{}]", id.value());
            return;
        }

        EterBase::ModernLogger::Info("SkillService::SetSkillLevel zaktualizowano. SkillId: {}, Level: {}, MasterType: {}", 
            id.value(), level, masterType);

        auto& skillView = m_skills[id];
        skillView.skillId = id;
        skillView.level = level;
        skillView.masterType = masterType;

        Core::EventBus::GetInstance().Publish(SkillLevelChangedEvent{id, level, masterType});
    }
}
