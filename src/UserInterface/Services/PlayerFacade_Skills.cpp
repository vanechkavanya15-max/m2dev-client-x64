#include "../StdAfx.h"
#include "ISkillService.h"
#include "../PythonPlayer.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "../Core/EventBus.h"
#include <algorithm>
#include <optional>

namespace UserInterface::Services
{
    class PlayerFacadeSkillBridge : public ISkillService
    {
    public:
        virtual ~PlayerFacadeSkillBridge() = default;

        void SetSkillLevel(EterBase::SkillId id, uint8_t level, uint8_t masterType) override
        {
            CPythonPlayer::Instance().SetSkillLevel_(id.Get(), masterType, level);
            EterBase::ModernLogger::Debug("PlayerFacadeSkillBridge: SetSkillLevel id={} level={} masterType={}", id.Get(), level, masterType);
        }

        void SetSkillCooltime(EterBase::SkillId id, float duration) override
        {
            CPythonPlayer::Instance().SetSkillCoolTime(id.Get());
            EterBase::ModernLogger::Debug("PlayerFacadeSkillBridge: SetSkillCooltime id={} duration={}", id.Get(), duration);
        }

        bool IsSkillCooltime(EterBase::SkillId id) const override
        {
            DWORD slotIndex = 0;
            if (CPythonPlayer::Instance().FindSkillSlotIndexBySkillIndex(id.Get(), &slotIndex))
            {
                return CPythonPlayer::Instance().IsSkillCoolTime(slotIndex) != FALSE;
            }
            return false;
        }

        float GetSkillCooltimeRemaining(EterBase::SkillId id) const override
        {
            DWORD slotIndex = 0;
            if (CPythonPlayer::Instance().FindSkillSlotIndexBySkillIndex(id.Get(), &slotIndex))
            {
                if (CPythonPlayer::Instance().IsSkillCoolTime(slotIndex))
                {
                    float total = CPythonPlayer::Instance().GetSkillCoolTime(slotIndex);
                    float elapsed = CPythonPlayer::Instance().GetSkillElapsedCoolTime(slotIndex);
                    return std::max(0.0f, total - elapsed);
                }
            }
            return 0.0f;
        }

        std::optional<PlayerSkillView> GetSkill(EterBase::SkillId id) const override
        {
            DWORD slotIndex = 0;
            if (CPythonPlayer::Instance().FindSkillSlotIndexBySkillIndex(id.Get(), &slotIndex))
            {
                PlayerSkillView view;
                view.skillId = id;
                view.level = static_cast<uint8_t>(CPythonPlayer::Instance().GetSkillLevel(slotIndex));
                view.masterType = static_cast<uint8_t>(CPythonPlayer::Instance().GetSkillGrade(slotIndex));
                view.totalCooltime = CPythonPlayer::Instance().GetSkillCoolTime(slotIndex);
                
                if (CPythonPlayer::Instance().IsSkillCoolTime(slotIndex))
                {
                    float elapsed = CPythonPlayer::Instance().GetSkillElapsedCoolTime(slotIndex);
                    view.cooltimeRemaining = std::max(0.0f, view.totalCooltime - elapsed);
                }
                else
                {
                    view.cooltimeRemaining = 0.0f;
                }
                
                view.isActived = (CPythonPlayer::Instance().IsSkillActive(slotIndex) != FALSE);
                return view;
            }
            return std::nullopt;
        }

        void UpdateCooltimes(float /*deltaTime*/) override
        {
            // Handled natively by PythonPlayer / CTimer
        }

        void Clear() override
        {
            CPythonPlayer::Instance().NEW_ClearSkillData(true);
            EterBase::ModernLogger::Debug("PlayerFacadeSkillBridge: Cleared skill data");
        }
    };
}
