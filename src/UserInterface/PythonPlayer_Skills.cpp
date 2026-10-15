#include "StdAfx.h"
#include "PythonPlayer.h"
#include "PythonCharacterManager.h"
#include "PythonApplication.h"
#include "PythonTextTail.h"
#include "EterBase/Timer.h"
#include "Services/ISkillService.h"
#include "Services/SkillService.h"
#include "Client/Bridge/StranglerFacade.h"

bool CPythonPlayer::AffectIndexToSkillIndex(DWORD dwAffectIndex, DWORD * pdwSkillIndex)
{
	if (m_kMap_dwAffectIndexToSkillIndex.end() == m_kMap_dwAffectIndexToSkillIndex.find(dwAffectIndex))
		return false;

	auto it = m_kMap_dwAffectIndexToSkillIndex.find(dwAffectIndex);
	if (it == m_kMap_dwAffectIndexToSkillIndex.end())
		return false;

	*pdwSkillIndex = it->second;
	return true;
}

bool CPythonPlayer::AffectIndexToSkillSlotIndex(UINT uAffect, DWORD* pdwSkillSlotIndex)
{
	auto it = m_kMap_dwAffectIndexToSkillIndex.find(uAffect);
	if (it == m_kMap_dwAffectIndexToSkillIndex.end())
		return false;

	return GetSkillSlotIndex(it->second, pdwSkillSlotIndex);
}

bool CPythonPlayer::__IsUsingChargeSkill()
{
	CInstanceBase * pkInstMain = NEW_GetMainActorPtr();
	if (!pkInstMain)
		return false;

	if (__CheckDashAffect(*pkInstMain))
		return true;

	if (MODE_USE_SKILL != m_eReservedMode)
		return false;

	if (m_dwSkillSlotIndexReserved >= SKILL_MAX_NUM)
		return false;

	TSkillInstance & rkSkillInst = m_aSkill[m_dwSkillSlotIndexReserved];

	CPythonSkill::TSkillData * pSkillData;
	if (!CPythonSkill::Instance().GetSkillData(rkSkillInst.dwIndex, &pSkillData))
		return false;

	return pSkillData->IsChargeSkill() ? true : false;
}

void CPythonPlayer::ResetSkillCoolTimes()
{
	for (int i = 0; i < SKILL_MAX_NUM; ++i)
		ResetSkillCoolTimeForSlot(i);
}

void CPythonPlayer::ResetSkillCoolTimeForSlot(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return;

	TSkillInstance& rkSkillInst = m_aSkill[dwSlotIndex];

	// If this skill is a toggle and currently active, deactivate it so UI/state is consistent.
	// __DeactivateSkillSlot is a private/protected helper on this class.
	if (IsToggleSkill(dwSlotIndex) && IsSkillActive(dwSlotIndex))
		__DeactivateSkillSlot(dwSlotIndex);

	// If nothing to clear, skip
	if (!rkSkillInst.fLastUsedTime && !rkSkillInst.fCoolTime)
		return;

	// Clear cooldown timers
	rkSkillInst.fLastUsedTime = rkSkillInst.fCoolTime = 0.0f;

	// Get the actual skill type to clear cooldowns from the correct storage
	DWORD dwSkillType = CPythonSkill::SKILL_TYPE_ACTIVE;  // default
	DWORD dwSkillIndex = rkSkillInst.dwIndex;
	CPythonSkill::TSkillData* pSkillData = NULL;

	if (dwSkillIndex != 0 && CPythonSkill::Instance().GetSkillData(dwSkillIndex, &pSkillData))
	{
		dwSkillType = pSkillData->byType;
	}

	if (dwSkillType == CPythonSkill::SKILL_TYPE_ACTIVE)
	{
		for (int iGrade = 0; iGrade < CPythonSkill::SKILL_GRADE_COUNT; ++iGrade)
		{
			UI::CWindowManager::Instance().ClearStoredSlotCoolTimeInAllSlotWindows(
				dwSkillType,
				dwSlotIndex + iGrade * CPythonSkill::SKILL_GRADE_STEP_COUNT);
		}
	}
	else
	{
		UI::CWindowManager::Instance().ClearStoredSlotCoolTimeInAllSlotWindows(dwSkillType, dwSlotIndex);
	}

	// Inform Python/UI which slot was cleared
	PyCallClassMemberFunc(m_ppyGameWindow, "SkillClearCoolTime", Py_BuildValue("(i)", (int)dwSlotIndex));
}

void CPythonPlayer::ResetHorseSkillCoolTime(DWORD dwSkillIndex, DWORD dwVisualSlotIndex)
{
    // Clear both the source slot (137-140) and the visual slot (0-3)
    // so RestoreSlotCoolTime won't have anything to restore
    
    UI::CWindowManager::Instance().ClearStoredSlotCoolTimeInAllSlotWindows(
        CPythonSkill::SKILL_TYPE_HORSE, 
        dwSkillIndex);
    
    UI::CWindowManager::Instance().ClearStoredSlotCoolTimeInAllSlotWindows(
        CPythonSkill::SKILL_TYPE_HORSE, 
        dwVisualSlotIndex);
}

void CPythonPlayer::SetSkill(DWORD dwSlotIndex, DWORD dwSkillIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return;

	m_aSkill[dwSlotIndex].dwIndex = dwSkillIndex;
	m_skillSlotDict[dwSkillIndex] = dwSlotIndex;
	if (dwSkillIndex != 0)
	{
		GetSkillDomain().RegisterSkill(Client::Gameplay::SkillId(dwSkillIndex), 0);
	}
}

int CPythonPlayer::GetSkillIndex(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0;

	return m_aSkill[dwSlotIndex].dwIndex;
}

bool CPythonPlayer::GetSkillSlotIndex(DWORD dwSkillIndex, DWORD* pdwSlotIndex)
{
	std::map<DWORD, DWORD>::iterator f=m_skillSlotDict.find(dwSkillIndex);
	if (m_skillSlotDict.end()==f)
	{
		return false;
	}

	*pdwSlotIndex=f->second;

	return true;
}

int CPythonPlayer::GetSkillGrade(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0;

	return m_aSkill[dwSlotIndex].iGrade;
}

int CPythonPlayer::GetSkillLevel(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0;

	return m_aSkill[dwSlotIndex].iLevel;
}

float CPythonPlayer::GetSkillCurrentEfficientPercentage(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0;

	return m_aSkill[dwSlotIndex].fcurEfficientPercentage;
}

float CPythonPlayer::GetSkillNextEfficientPercentage(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0;

	return m_aSkill[dwSlotIndex].fnextEfficientPercentage;
}

void CPythonPlayer::SetSkillLevel(DWORD dwSlotIndex, DWORD dwSkillLevel)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return;

	m_aSkill[dwSlotIndex].iGrade = -1;
	m_aSkill[dwSlotIndex].iLevel = dwSkillLevel;
	DWORD dwSkillIndex = m_aSkill[dwSlotIndex].dwIndex;
	if (dwSkillIndex != 0)
	{
		GetSkillDomain().SetSkillLevel(Client::Gameplay::SkillId(dwSkillIndex), static_cast<Client::Gameplay::SkillLevel>(dwSkillLevel));
	}
}

void CPythonPlayer::SetSkillLevel_(DWORD dwSkillIndex, DWORD dwSkillGrade, DWORD dwSkillLevel)
{
	DWORD dwSlotIndex;
	if (!GetSkillSlotIndex(dwSkillIndex, &dwSlotIndex))
		return;

	if (dwSlotIndex >= SKILL_MAX_NUM)
		return;

	switch (dwSkillGrade)
	{
		case 0:
			m_aSkill[dwSlotIndex].iGrade = dwSkillGrade;
			m_aSkill[dwSlotIndex].iLevel = dwSkillLevel;
			break;
		case 1:
			m_aSkill[dwSlotIndex].iGrade = dwSkillGrade;
			m_aSkill[dwSlotIndex].iLevel = dwSkillLevel-20+1;
			break;
		case 2:
			m_aSkill[dwSlotIndex].iGrade = dwSkillGrade;
			m_aSkill[dwSlotIndex].iLevel = dwSkillLevel-30+1;
			break;
		case 3:
			m_aSkill[dwSlotIndex].iGrade = dwSkillGrade;
			m_aSkill[dwSlotIndex].iLevel = dwSkillLevel-40+1;
			break;
	}

	const DWORD SKILL_MAX_LEVEL = 40;

	if (dwSkillLevel>SKILL_MAX_LEVEL)
	{
		m_aSkill[dwSlotIndex].fcurEfficientPercentage = 0.0f;
		m_aSkill[dwSlotIndex].fnextEfficientPercentage = 0.0f;

		TraceError("CPythonPlayer::SetSkillLevel(SlotIndex=%d, SkillLevel=%d)", dwSlotIndex, dwSkillLevel);
		return;
	}

	if (m_aSkill[dwSlotIndex].iLevel <= 0)
	{
		ResetSkillCoolTimeForSlot(dwSlotIndex);
	}

	m_aSkill[dwSlotIndex].fcurEfficientPercentage = GetSkillPower(dwSkillLevel)/100.0f;
	m_aSkill[dwSlotIndex].fnextEfficientPercentage = GetSkillPower(dwSkillLevel+1)/100.0f;

	GetSkillDomain().SetSkillMastery(Client::Gameplay::SkillId(dwSkillIndex), static_cast<uint8_t>(dwSkillGrade), static_cast<uint8_t>(m_aSkill[dwSlotIndex].iLevel));
	GetSkillDomain().SetSkillLevel(Client::Gameplay::SkillId(dwSkillIndex), static_cast<Client::Gameplay::SkillLevel>(dwSkillLevel));
	UserInterface::Services::SkillService::Instance().SetSkillLevel(EterBase::SkillId(dwSkillIndex), static_cast<uint8_t>(dwSkillLevel), static_cast<uint8_t>(dwSkillGrade));
}

void CPythonPlayer::SetSkillCoolTime(DWORD dwSkillIndex, float fOverrideDuration)
{
	DWORD dwSlotIndex;
	if (!GetSkillSlotIndex(dwSkillIndex, &dwSlotIndex))
	{
		Tracenf("CPythonPlayer::SetSkillCoolTime(dwSkillIndex=%d) - FIND SLOT ERROR", dwSkillIndex);
		return;
	}

	if (dwSlotIndex>=SKILL_MAX_NUM)
	{
		Tracenf("CPythonPlayer::SetSkillCoolTime(dwSkillIndex=%d) - dwSlotIndex=%d/%d OUT OF RANGE", dwSkillIndex, dwSlotIndex, SKILL_MAX_NUM);
		return;
	}

	m_aSkill[dwSlotIndex].isCoolTime=true;
	m_aSkill[dwSlotIndex].fLastUsedTime = CTimer::Instance().GetCurrentSecond();
	float fCoolTime = (fOverrideDuration > 0.0f) ? fOverrideDuration : m_aSkill[dwSlotIndex].fCoolTime;
	m_aSkill[dwSlotIndex].fCoolTime = fCoolTime;
	uint32_t coolTimeMs = fCoolTime > 0.0f ? static_cast<uint32_t>(fCoolTime * 1000.0f) : 0;
	if (coolTimeMs > 0)
	{
		GetSkillDomain().StartCooldown(Client::Gameplay::SkillId(dwSkillIndex), std::chrono::milliseconds(coolTimeMs));
	}
	UserInterface::Core::EventBus::GetInstance().Publish(
		UserInterface::Services::SkillCooltimeStartEvent{
			EterBase::SkillId(dwSkillIndex),
			fCoolTime
		}
	);
}

void CPythonPlayer::EndSkillCoolTime(DWORD dwSkillIndex)
{
	DWORD dwSlotIndex;
	if (!GetSkillSlotIndex(dwSkillIndex, &dwSlotIndex))
	{
		Tracenf("CPythonPlayer::EndSkillCoolTime(dwSkillIndex=%d) - FIND SLOT ERROR", dwSkillIndex);
		return;
	}

	if (dwSlotIndex>=SKILL_MAX_NUM)
	{
		Tracenf("CPythonPlayer::EndSkillCoolTime(dwSkillIndex=%d) - dwSlotIndex=%d/%d OUT OF RANGE", dwSkillIndex, dwSlotIndex, SKILL_MAX_NUM);
		return;
	}

	m_aSkill[dwSlotIndex].isCoolTime=false;
	GetSkillDomain().ResetCooldown(Client::Gameplay::SkillId(dwSkillIndex));
}

float CPythonPlayer::GetSkillCoolTime(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0.0f;

	return m_aSkill[dwSlotIndex].fCoolTime;
}

float CPythonPlayer::GetSkillElapsedCoolTime(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0.0f;

	return CTimer::Instance().GetCurrentSecond() - m_aSkill[dwSlotIndex].fLastUsedTime;
}

void CPythonPlayer::__ActivateSkillSlot(DWORD dwSlotIndex)
{
	if (dwSlotIndex>=SKILL_MAX_NUM)
	{
		Tracenf("CPythonPlayer::ActivavteSkill(dwSlotIndex=%d/%d) - OUT OF RANGE", dwSlotIndex, SKILL_MAX_NUM);
		return;
	}

	m_aSkill[dwSlotIndex].bActive = TRUE;
	DWORD dwSkillIndex = m_aSkill[dwSlotIndex].dwIndex;
	if (dwSkillIndex != 0)
	{
		GetSkillDomain().ToggleSkill(Client::Gameplay::SkillId(dwSkillIndex), true);
	}
	PyCallClassMemberFunc(m_ppyGameWindow, "ActivateSkillSlot", Py_BuildValue("(i)", dwSlotIndex));
}

void CPythonPlayer::__DeactivateSkillSlot(DWORD dwSlotIndex)
{
	if (dwSlotIndex>=SKILL_MAX_NUM)
	{
		Tracenf("CPythonPlayer::DeactivavteSkill(dwSlotIndex=%d/%d) - OUT OF RANGE", dwSlotIndex, SKILL_MAX_NUM);
		return;
	}

	m_aSkill[dwSlotIndex].bActive = FALSE;
	DWORD dwSkillIndex = m_aSkill[dwSlotIndex].dwIndex;
	if (dwSkillIndex != 0)
	{
		GetSkillDomain().ToggleSkill(Client::Gameplay::SkillId(dwSkillIndex), false);
	}
	PyCallClassMemberFunc(m_ppyGameWindow, "DeactivateSkillSlot", Py_BuildValue("(i)", dwSlotIndex));
}

BOOL CPythonPlayer::IsSkillCoolTime(DWORD dwSlotIndex)
{
	if (!__CheckRestSkillCoolTime(dwSlotIndex))
		return FALSE;

	return TRUE;
}

BOOL CPythonPlayer::IsSkillActive(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return FALSE;

	return m_aSkill[dwSlotIndex].bActive;
}

BOOL CPythonPlayer::IsToggleSkill(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return FALSE;

	DWORD dwSkillIndex = m_aSkill[dwSlotIndex].dwIndex;

	CPythonSkill::TSkillData * pSkillData;
	if (!CPythonSkill::Instance().GetSkillData(dwSkillIndex, &pSkillData))
		return FALSE;

	return pSkillData->IsToggleSkill();
}

BOOL CPythonPlayer::__ToggleCoolTime()
{
	m_sysIsCoolTime = 1 - m_sysIsCoolTime;
	return m_sysIsCoolTime;
}

BOOL CPythonPlayer::__ToggleLevelLimit()
{
	m_sysIsLevelLimit = 1 - m_sysIsLevelLimit;
	return m_sysIsLevelLimit;
}

void CPythonPlayer::NEW_ClearSkillData(bool bAll)
{
	std::map<DWORD, DWORD>::iterator it;

	for (it = m_skillSlotDict.begin(); it != m_skillSlotDict.end();)
	{
		CPythonSkill::TSkillData* data = nullptr;

		if (!CPythonSkill::Instance().GetSkillData(it->first, &data))
		{
			++it;
			continue;
		}

		if (bAll || (data->byType != CPythonSkill::SKILL_TYPE_SUPPORT && data->byType != CPythonSkill::SKILL_TYPE_HORSE && data->byType != CPythonSkill::SKILL_TYPE_GUILD))
			it = m_skillSlotDict.erase(it);
		else
			++it;
	}

	for (int i = 0; i < SKILL_MAX_NUM; ++i)
	{
		DWORD dwSkillIndex = m_aSkill[i].dwIndex;
		CPythonSkill::TSkillData* pSkillData = NULL;

		// Skip empty slots
		if (dwSkillIndex == 0)
			continue;

		if (!CPythonSkill::Instance().GetSkillData(dwSkillIndex, &pSkillData))
			continue;

		// If not clearing all, skip persistent skill types (SUPPORT, HORSE, GUILD)
		if (!bAll && (pSkillData->byType == CPythonSkill::SKILL_TYPE_SUPPORT || 
					   pSkillData->byType == CPythonSkill::SKILL_TYPE_HORSE || 
					   pSkillData->byType == CPythonSkill::SKILL_TYPE_GUILD))
			continue;

		ZeroMemory(&m_aSkill[i], sizeof(TSkillInstance));
	}

	for (int j = 0; j < SKILL_MAX_NUM; ++j)
	{
		// 2004.09.30.myevan.스킬갱신시 스킬 포인트업[+] 버튼이 안나와 처리
		m_aSkill[j].iGrade = 0;
		m_aSkill[j].fcurEfficientPercentage = 0.0f;
		m_aSkill[j].fnextEfficientPercentage = 0.05f;

		m_aSkill[j].isCoolTime = false;
		m_aSkill[j].fCoolTime = 0.0f;
		m_aSkill[j].fLastUsedTime = 0.0f;

		//ResetSkillCoolTimeForSlot(j);
		for (int iGrade = 0; iGrade < CPythonSkill::SKILL_GRADE_COUNT; ++iGrade)
		{
			UI::CWindowManager::Instance().ClearStoredSlotCoolTimeInAllSlotWindows(
				CPythonSkill::SKILL_TYPE_ACTIVE,
				j + iGrade * CPythonSkill::SKILL_GRADE_STEP_COUNT);
		}
	}

	if (bAll)
	{
		GetSkillDomain().Clear();
	}

	if (m_ppyGameWindow)
		PyCallClassMemberFunc(m_ppyGameWindow, "BINARY_CheckGameButton", Py_BuildValue("()"));
}

void CPythonPlayer::ClearSkillDict()
{
	// ClearSkillDict
	m_skillSlotDict.clear();
	ClearDictionary();

	// Game End - Player Data Reset
	m_isOpenPrivateShop = false;
	m_isObserverMode = false;

	m_isConsumingStamina = FALSE;
	m_fConsumeStaminaPerSec = 0.0f;
	m_fCurrentStamina = 0.0f;

	__ClearAutoAttackTargetActorID();
}
