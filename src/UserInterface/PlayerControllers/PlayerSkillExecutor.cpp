#include "../StdAfx.h"
#include "PlayerSkillExecutor.h"

#include "../InstanceBase.h"
#include "EterBase/Timer.h"

namespace UserInterface::PlayerControllers
{
    PlayerSkillExecutor::PlayerSkillExecutor(
        Contracts::IActorProvider* pActorProvider,
        Contracts::INetworkService* pNetworkService)
        : m_pActorProvider(pActorProvider)
        , m_pNetworkService(pNetworkService)
        , m_dwcurSkillSlotIndex(0)
        , m_dwSkillSlotIndexReserved(0)
        , m_dwSkillRangeReserved(0)
        , m_dwSkillTargetVIDReserved(0)
        , m_isSkillReserved(false)
    {
        for (size_t i = 0; i < MAX_SKILL_COUNT; ++i)
        {
            m_aSkill[i] = SSkillSlotData();
        }
    }

    void PlayerSkillExecutor::SetSkillSlot(DWORD dwSlotIndex, DWORD dwSkillIndex, int iGrade, int iLevel, float fEfficiency)
    {
        if (dwSlotIndex >= MAX_SKILL_COUNT)
            return;

        m_aSkill[dwSlotIndex].dwIndex = dwSkillIndex;
        m_aSkill[dwSlotIndex].iGrade = iGrade;
        m_aSkill[dwSlotIndex].iLevel = iLevel;
        m_aSkill[dwSlotIndex].fcurEfficientPercentage = fEfficiency;
    }

    const PlayerSkillExecutor::SSkillSlotData* PlayerSkillExecutor::GetSkillSlotData(DWORD dwSlotIndex) const
    {
        if (dwSlotIndex >= MAX_SKILL_COUNT)
            return nullptr;

        return &m_aSkill[dwSlotIndex];
    }

    PlayerSkillExecutor::SSkillSlotData* PlayerSkillExecutor::GetSkillSlotData(DWORD dwSlotIndex)
    {
        if (dwSlotIndex >= MAX_SKILL_COUNT)
            return nullptr;

        return &m_aSkill[dwSlotIndex];
    }

    bool PlayerSkillExecutor::CheckRestSkillCoolTime(DWORD dwSkillSlotIndex) const
    {
        if (dwSkillSlotIndex >= MAX_SKILL_COUNT)
            return false;

        const SSkillSlotData& rkSkillInst = m_aSkill[dwSkillSlotIndex];
        if (rkSkillInst.fCoolTime <= 0.0f)
            return false;

        float fCurrentTime = CTimer::Instance().GetCurrentSecond();
        if (fCurrentTime - rkSkillInst.fLastUsedTime < rkSkillInst.fCoolTime)
            return true;

        return false;
    }

    void PlayerSkillExecutor::RunCoolTime(DWORD dwSkillSlotIndex, float fBaseCoolTime, int iCastingSpeed)
    {
        if (dwSkillSlotIndex >= MAX_SKILL_COUNT)
            return;

        SSkillSlotData& rkSkillInst = m_aSkill[dwSkillSlotIndex];
        rkSkillInst.fCoolTime = fBaseCoolTime;
        rkSkillInst.fLastUsedTime = CTimer::Instance().GetCurrentSecond();

        int iSpd = 100 - iCastingSpeed;
        if (iSpd > 0)
            iSpd = 100 + iSpd;
        else if (iSpd < 0)
            iSpd = 10000 / (100 - iSpd);
        else
            iSpd = 100;

        rkSkillInst.fCoolTime = rkSkillInst.fCoolTime * float(iSpd) / 100.0f;
        rkSkillInst.isCoolTime = TRUE;
    }

    float PlayerSkillExecutor::GetSkillCoolTime(DWORD dwSkillSlotIndex) const
    {
        if (dwSkillSlotIndex >= MAX_SKILL_COUNT)
            return 0.0f;

        return m_aSkill[dwSkillSlotIndex].fCoolTime;
    }

    float PlayerSkillExecutor::GetSkillElapsedCoolTime(DWORD dwSkillSlotIndex) const
    {
        if (dwSkillSlotIndex >= MAX_SKILL_COUNT)
            return 0.0f;

        float fCurrentTime = CTimer::Instance().GetCurrentSecond();
        return fCurrentTime - m_aSkill[dwSkillSlotIndex].fLastUsedTime;
    }

    void PlayerSkillExecutor::ResetSkillCoolTimes()
    {
        for (size_t i = 0; i < MAX_SKILL_COUNT; ++i)
        {
            m_aSkill[i].fCoolTime = 0.0f;
            m_aSkill[i].fLastUsedTime = 0.0f;
            m_aSkill[i].isCoolTime = FALSE;
        }
    }

    void PlayerSkillExecutor::ResetSkillCoolTimeForSlot(DWORD dwSkillSlotIndex)
    {
        if (dwSkillSlotIndex >= MAX_SKILL_COUNT)
            return;

        m_aSkill[dwSkillSlotIndex].fCoolTime = 0.0f;
        m_aSkill[dwSkillSlotIndex].fLastUsedTime = 0.0f;
        m_aSkill[dwSkillSlotIndex].isCoolTime = FALSE;
    }

    DWORD PlayerSkillExecutor::GetSkillTargetRange(DWORD dwSkillBaseRange, DWORD dwBowDistance) const
    {
        return dwSkillBaseRange + dwBowDistance * 100;
    }

    bool PlayerSkillExecutor::CheckSkillTargetRange(DWORD dwSkillSlotIndex, DWORD dwTargetVID, DWORD dwSkillBaseRange) const
    {
        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        CInstanceBase* pkInstTarget = m_pActorProvider ? m_pActorProvider->GetInstance(dwTargetVID) : nullptr;
        if (!pkInstMain || !pkInstTarget)
            return false;

        float fDist = pkInstMain->GetDistance(pkInstTarget);
        return (fDist <= float(dwSkillBaseRange));
    }

    bool PlayerSkillExecutor::ProcessEnemySkillTargetRange(
        DWORD dwSkillSlotIndex,
        DWORD dwTargetVID,
        float fTargetDistance,
        DWORD dwSkillRange,
        bool bIsChargeSkill)
    {
        float fSkillTargetRange = float(dwSkillRange);
        if (fSkillTargetRange <= 0.0f)
            return true;

        if (fTargetDistance >= fSkillTargetRange)
        {
            if (bIsChargeSkill)
            {
                if (!IsReservedUseSkill(dwSkillSlotIndex))
                {
                    if (dwSkillSlotIndex < MAX_SKILL_COUNT)
                        SendUseSkillPacket(m_aSkill[dwSkillSlotIndex].dwIndex, 0);
                }
            }

            ReserveUseSkill(dwTargetVID, dwSkillSlotIndex, dwSkillRange);
            return false;
        }

        return true;
    }

    bool PlayerSkillExecutor::UseSkill(DWORD dwSkillSlotIndex, DWORD dwTargetVID)
    {
        if (dwSkillSlotIndex >= MAX_SKILL_COUNT)
            return false;

        if (!CheckSkillUsable(dwSkillSlotIndex))
            return false;

        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return false;

        if (pkInstMain->IsUsingSkill())
            return false;

        ClearReservedSkill();

        SendUseSkillPacket(m_aSkill[dwSkillSlotIndex].dwIndex, dwTargetVID);
        RunCoolTime(dwSkillSlotIndex, m_aSkill[dwSkillSlotIndex].fCoolTime);
        return true;
    }

    void PlayerSkillExecutor::UseCurrentSkill()
    {
        UseSkill(m_dwcurSkillSlotIndex);
    }

    void PlayerSkillExecutor::UseChargeSkill(DWORD dwSkillSlotIndex)
    {
        UseSkill(dwSkillSlotIndex, 0);
    }

    bool PlayerSkillExecutor::IsUsingChargeSkill() const
    {
        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return false;

        if (pkInstMain->IsAffect(CInstanceBase::AFFECT_DASH))
            return true;

        return false;
    }

    void PlayerSkillExecutor::ReserveUseSkill(DWORD dwTargetVID, DWORD dwSkillSlotIndex, DWORD dwRange)
    {
        m_isSkillReserved = true;
        m_dwSkillTargetVIDReserved = dwTargetVID;
        m_dwSkillSlotIndexReserved = dwSkillSlotIndex;
        m_dwSkillRangeReserved = dwRange;

        if (m_dwSkillRangeReserved > 100)
            m_dwSkillRangeReserved -= 10;
    }

    bool PlayerSkillExecutor::IsReservedUseSkill(DWORD dwSkillSlotIndex) const
    {
        if (!m_isSkillReserved)
            return false;

        return (m_dwSkillSlotIndexReserved == dwSkillSlotIndex);
    }

    void PlayerSkillExecutor::ClearReservedSkill()
    {
        m_isSkillReserved = false;
        m_dwSkillTargetVIDReserved = 0;
        m_dwSkillSlotIndexReserved = 0;
        m_dwSkillRangeReserved = 0;
    }

    bool PlayerSkillExecutor::CheckSkillUsable(DWORD dwSkillSlotIndex) const
    {
        if (dwSkillSlotIndex >= MAX_SKILL_COUNT)
            return false;

        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return false;

        if (!pkInstMain->CanUseSkill())
            return false;

        if (CheckRestSkillCoolTime(dwSkillSlotIndex))
            return false;

        return true;
    }

    void PlayerSkillExecutor::RegisterAffectSkillMap(DWORD dwAffectIndex, DWORD dwSkillIndex)
    {
        m_kMap_dwAffectIndexToSkillIndex[dwAffectIndex] = dwSkillIndex;
    }

    bool PlayerSkillExecutor::AffectIndexToSkillIndex(DWORD dwAffectIndex, DWORD* pdwSkillIndex) const
    {
        auto it = m_kMap_dwAffectIndexToSkillIndex.find(dwAffectIndex);
        if (it != m_kMap_dwAffectIndexToSkillIndex.end())
        {
            if (pdwSkillIndex)
                *pdwSkillIndex = it->second;
            return true;
        }

        return false;
    }

    void PlayerSkillExecutor::ClearAffectSkillMap()
    {
        m_kMap_dwAffectIndexToSkillIndex.clear();
    }

    bool PlayerSkillExecutor::SendUseSkillPacket(DWORD dwSkillIndex, DWORD dwTargetVID)
    {
        if (m_pNetworkService)
            return m_pNetworkService->SendUseSkillPacket(dwSkillIndex, dwTargetVID);

        return false;
    }

    void PlayerSkillExecutor::Update()
    {
        // Aktualizacja i weryfikacja zarezerwowanej umiejetnosci
        if (m_isSkillReserved && m_dwSkillTargetVIDReserved != 0)
        {
            if (m_pActorProvider && !m_pActorProvider->IsActorAlive(m_dwSkillTargetVIDReserved))
            {
                ClearReservedSkill();
            }
        }
    }
}
