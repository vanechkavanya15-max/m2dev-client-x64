#include "StdAfx.h"
#include "PythonPlayer.h"
#include "PythonApplication.h"
#include "Services/IPlayerStatsService.h"
#include "Services/PlayerStatsService.h"
#include "Client/Bridge/StranglerFacade.h"

int64_t CPythonPlayer::GetPointChange(DWORD dwType)
{
	return GetStatus64(dwType);
}

int64_t CPythonPlayer::GetEXP() const
{
	return GetStatus64(POINT_EXP);
}

int64_t CPythonPlayer::GetGold() const
{
	return GetStatus64(POINT_GOLD);
}

int64_t CPythonPlayer::GetCheque() const
{
	return 0;
}

void CPythonPlayer::SetStatus(DWORD dwType, long lValue)
{
	SetStatus64(dwType, static_cast<int64_t>(lValue));
}

void CPythonPlayer::SetStatus64(DWORD dwType, int64_t lValue)
{
	if (dwType >= POINT_MAX_NUM)
	{
		assert(!" CPythonPlayer::SetStatus64 - Strange Status Type!");
		Tracef("CPythonPlayer::SetStatus64 - Set Status Type Error\n");
		return;
	}

	if (dwType == POINT_LEVEL)
	{
		CInstanceBase* pkPlayer = NEW_GetMainActorPtr();

		if (pkPlayer)
		{
			pkPlayer->SetLevel(static_cast<DWORD>(lValue));
			pkPlayer->UpdateTextTailLevel(static_cast<DWORD>(lValue));
		}
	}

	Client::Bridge::StranglerFacade::Instance().GetWorldContext().SetPoint(dwType, lValue);
	UserInterface::Services::PlayerStatsService::Instance().SetPoint(dwType, lValue);

	switch (dwType)
	{
		case POINT_LEVEL:
		case POINT_ST:
		case POINT_DX:
		case POINT_IQ:
		case POINT_HT:
			__UpdateBattleStatus();
			break;
		default:
			break;
	}		
}

int64_t CPythonPlayer::GetStatus64(DWORD dwType) const
{
	if (dwType >= POINT_MAX_NUM)
	{
		assert(!" CPythonPlayer::GetStatus64 - Strange Status Type!");
		Tracef("CPythonPlayer::GetStatus64 - Get Status Type Error\n");
		return 0;
	}

	return Client::Bridge::StranglerFacade::Instance().GetWorldContext().GetPoint(dwType);
}

int CPythonPlayer::GetStatus(DWORD dwType)
{
	return static_cast<int>(GetStatus64(dwType));
}

void CPythonPlayer::NotifyChangePKMode()
{
	PyCallClassMemberFunc(m_ppyGameWindow, "OnChangePKMode", Py_BuildValue("()"));
}

void CPythonPlayer::StartStaminaConsume(DWORD dwConsumePerSec, DWORD dwCurrentStamina)
{
	m_isConsumingStamina = TRUE;
	m_fConsumeStaminaPerSec = float(dwConsumePerSec);
	m_fCurrentStamina = float(dwCurrentStamina);

	SetStatus(POINT_STAMINA, dwCurrentStamina);
	m_movementController.StartStaminaConsume(dwConsumePerSec, dwCurrentStamina);
}

void CPythonPlayer::StopStaminaConsume(DWORD dwCurrentStamina)
{
	m_isConsumingStamina = FALSE;
	m_fConsumeStaminaPerSec = 0.0f;
	m_fCurrentStamina = float(dwCurrentStamina);

	SetStatus(POINT_STAMINA, dwCurrentStamina);
	m_movementController.StopStaminaConsume(dwCurrentStamina);
}

void CPythonPlayer::SetAutoPotionInfo(int type, const SAutoPotionInfo& info)
{
	if (type < 0 || type >= AUTO_POTION_TYPE_NUM)
		return;

	m_kAutoPotionInfo[type] = info;

	UserInterface::PlayerControllers::PlayerItemController::SAutoPotionInfo itemInfo;
	itemInfo.bActivated = info.bActivated;
	itemInfo.totalAmount = info.totalAmount;
	itemInfo.currentAmount = info.currentAmount;
	m_itemController.SetAutoPotionInfo(type, itemInfo);
}

DWORD CPythonPlayer::GetPKMode()
{
	CInstanceBase * pInstance = NEW_GetMainActorPtr();
	if (!pInstance)
		return 0;

	return pInstance->GetPKMode();
}
