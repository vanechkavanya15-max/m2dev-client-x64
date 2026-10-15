#include "StdAfx.h"
#include "PythonPlayer.h"
#include "PythonCharacterManager.h"
#include "PythonNetworkStream.h"
#include "Contracts/ICombatPolicy.h"

void CPythonPlayer::SetWeaponPower(DWORD dwMinPower, DWORD dwMaxPower, DWORD dwMinMagicPower, DWORD dwMaxMagicPower, DWORD dwAddPower)
{
	m_dwWeaponMinPower = dwMinPower;
	m_dwWeaponMaxPower = dwMaxPower;
	m_dwWeaponMinMagicPower = dwMinMagicPower;
	m_dwWeaponMaxMagicPower = dwMaxMagicPower;
	m_dwWeaponAddPower = dwAddPower;

	__UpdateBattleStatus();
}

void CPythonPlayer::SetRace(DWORD dwRace)
{
	m_dwRace = dwRace;
}

DWORD CPythonPlayer::GetRace()
{
	return m_dwRace;
}

DWORD CPythonPlayer::__GetRaceStat()
{
	switch (GetRace())
	{
		case MAIN_RACE_WARRIOR_M:
		case MAIN_RACE_WARRIOR_W:
			return GetStatus(POINT_ST);
		case MAIN_RACE_ASSASSIN_M:
		case MAIN_RACE_ASSASSIN_W:
			return GetStatus(POINT_DX);
		case MAIN_RACE_SURA_M:
		case MAIN_RACE_SURA_W:
			return GetStatus(POINT_ST);
		case MAIN_RACE_SHAMAN_M:
		case MAIN_RACE_SHAMAN_W:
			return GetStatus(POINT_IQ);
	}
	return GetStatus(POINT_ST);
}

DWORD CPythonPlayer::__GetLevelAtk()
{
	return GetStatus(POINT_LEVEL) * 2;
}

DWORD CPythonPlayer::__GetStatAtk()
{
	return (GetStatus(POINT_ST) * 4 + __GetRaceStat()) / 3;
}

DWORD CPythonPlayer::__GetWeaponAtk(DWORD dwWeaponPower)
{
	return dwWeaponPower * 2;
}

DWORD CPythonPlayer::__GetTotalAtk(DWORD dwWeaponPower, DWORD dwRefineBonus)
{
	DWORD dwLvAtk = __GetLevelAtk();
	DWORD dwStAtk = __GetStatAtk();
	DWORD dwWepAtk = __GetWeaponAtk(dwWeaponPower);

	DWORD dwTotalAtk = dwLvAtk + (dwStAtk + dwWepAtk) * (dwRefineBonus + 100) / 100;

	return dwTotalAtk;
}

DWORD CPythonPlayer::__GetHitRate()
{
	int src = GetStatus(POINT_DX) * 4;
	int dst = GetStatus(POINT_LEVEL) * 2 + 20;
	if (src + dst == 0)
		return 0;
	return src * 100 / (src + dst);
}

DWORD CPythonPlayer::__GetEvadeRate()
{
	return 0;
}

void CPythonPlayer::__UpdateBattleStatus()
{
	static thread_local bool s_isUpdating = false;
	static thread_local bool s_hasPendingUpdate = false;

	if (s_isUpdating)
	{
		s_hasPendingUpdate = true;
		return;
	}

	s_isUpdating = true;
	do
	{
		s_hasPendingUpdate = false;

		m_combatController.SetPlayerStats(
			GetStatus(POINT_LEVEL),
			GetStatus(POINT_ST),
			GetStatus(POINT_DX),
			GetStatus(POINT_IQ),
			GetStatus(POINT_HT)
		);
		m_combatController.SetRace(GetRace());
		m_combatController.SetWeaponPower(
			m_dwWeaponMinPower,
			m_dwWeaponMaxPower,
			m_dwWeaponMinMagicPower,
			m_dwWeaponMaxMagicPower,
			m_dwWeaponAddPower
		);

		SetStatus(POINT_MIN_WEP, m_dwWeaponMinPower + m_dwWeaponAddPower);
		SetStatus(POINT_MAX_WEP, m_dwWeaponMaxPower + m_dwWeaponAddPower);
		SetStatus(POINT_MIN_ATK, __GetTotalAtk(m_dwWeaponMinPower, m_dwWeaponAddPower));
		SetStatus(POINT_MAX_ATK, __GetTotalAtk(m_dwWeaponMaxPower, m_dwWeaponAddPower));
		SetStatus(POINT_HIT_RATE, __GetHitRate());
		SetStatus(POINT_EVADE_RATE, __GetEvadeRate());
	} while (s_hasPendingUpdate);

	s_isUpdating = false;
}

void CPythonPlayer::__Update_AutoAttack()
{
	if (0 == m_dwAutoAttackTargetVID)
		return;

	CInstanceBase* pkInstMain = NEW_GetMainActorPtr();
	if (!pkInstMain)
		return;

	// 탄환격 쓰고 달려가는 도중에는 스킵
	if (__IsUsingChargeSkill())
		return;

	CInstanceBase* pkInstVictim = NEW_FindActorPtr(m_dwAutoAttackTargetVID);
	if (!pkInstVictim)
	{
		__ClearAutoAttackTargetActorID();
	}
	else
	{
		if (pkInstVictim->IsDead())
		{
			__ClearAutoAttackTargetActorID();
		}
		else if (pkInstMain->IsMountingHorse() && !pkInstMain->CanAttackHorseLevel())
		{
			__ClearAutoAttackTargetActorID();
		}
		else if (pkInstMain->IsAttackableInstance(*pkInstVictim))
		{
			if (m_pCombatPolicy && !m_pCombatPolicy->CanAttackInstance(m_dwAutoAttackTargetVID))
			{
				return;
			}

			if (pkInstMain->IsSleep())
			{
				//TraceError("SKIP_AUTO_ATTACK_IN_SLEEPING");
			}
			else
			{
				__ReserveClickActor(m_dwAutoAttackTargetVID);
			}
		}
	}
}

void CPythonPlayer::__SendClickActorPacket(CInstanceBase& rkInstVictim)
{
	// 말을 타고 광산을 캐는 것에 대한 예외 처리
	CInstanceBase* pkInstMain = NEW_GetMainActorPtr();
	if (pkInstMain)
	if (pkInstMain->IsHoldingPickAxe())
	if (pkInstMain->IsMountingHorse())
	if (rkInstVictim.IsResource())
	{
		PyCallClassMemberFunc(m_ppyGameWindow, "OnCannotMining", Py_BuildValue("()"));
		return;
	}

	static DWORD s_dwNextTCPTime = 0;

	DWORD dwCurTime = ELTimer_GetMSec();

	if (dwCurTime >= s_dwNextTCPTime)
	{
		s_dwNextTCPTime = dwCurTime + 1000;

		CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();

		DWORD dwVictimVID = rkInstVictim.GetVirtualID();
		rkNetStream.SendOnClickPacket(dwVictimVID);
	}
}

bool CPythonPlayer::CanAttackInstance(DWORD dwTargetVID)
{
	if (m_pCombatPolicy && !m_pCombatPolicy->CanAttackInstance(dwTargetVID))
	{
		return false;
	}

	CInstanceBase* pkInstMain = NEW_GetMainActorPtr();
	if (!pkInstMain || pkInstMain->IsDead())
		return false;

	CInstanceBase* pkInstTarget = NEW_FindActorPtr(dwTargetVID);
	if (!pkInstTarget || pkInstTarget->IsDead())
		return false;

	if (pkInstMain->IsInSafe() || pkInstTarget->IsInSafe())
		return false;

	return pkInstMain->IsAttackableInstance(*pkInstTarget);
}

bool CPythonPlayer::AttackTarget(DWORD dwTargetVID)
{
	if (!CanAttackInstance(dwTargetVID))
		return false;

	if (m_pCombatPolicy && !m_pCombatPolicy->AttackTarget(dwTargetVID))
	{
		return false;
	}

	CInstanceBase* pkInstMain = NEW_GetMainActorPtr();
	CInstanceBase* pkInstTarget = NEW_FindActorPtr(dwTargetVID);
	if (!pkInstMain || !pkInstTarget)
		return false;

	if (pkInstMain->NEW_IsClickableDistanceDestInstance(*pkInstTarget))
	{
		pkInstMain->NEW_AttackToDestInstanceDirection(*pkInstTarget);
	}
	else
	{
		pkInstMain->NEW_MoveToDestInstanceDirection(*pkInstTarget);
	}
	return true;
}

void CPythonPlayer::UpdateAttackState(float fElapsedTime)
{
	if (m_pCombatPolicy != nullptr)
	{
		m_pCombatPolicy->UpdateAttackState(fElapsedTime);
	}

	m_combatController.UpdateAutoAttack(fElapsedTime);
	__Update_AutoAttack();
}
