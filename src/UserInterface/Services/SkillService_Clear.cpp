#include "../StdAfx.h"
#include "SkillService.h"
#include "../../EterBase/LogModern.h"

namespace UserInterface::Services
{
    void SkillService::Clear()
    {
        EterBase::ModernLogger::Info("SkillService::Clear - Resetting skill states and cooldowns.");
        m_skills.clear();
    }
}
