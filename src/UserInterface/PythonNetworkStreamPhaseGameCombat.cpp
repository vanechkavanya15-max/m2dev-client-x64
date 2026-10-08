#include "StdAfx.h"
#include "PythonNetworkStreamPhaseGameCombat.h"
#include "PythonNetworkStream.h"
#include "PythonCharacterManager.h"
#include "PythonPlayer.h"
#include "InstanceBase.h"
#include "Client/Network/CombatPacketCodec.h"
#include <span>

bool PhaseGameCombatBridge::HandleDamageInfo(CPythonNetworkStream* pStream, const TPacketGCDamageInfo& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Network::CombatPacketCodec::DecodeDamageInfo(span);
	if (!res.has_value())
		return false;

	const auto& DamageInfoPacket = res.value();

	CInstanceBase* pInstTarget = CPythonCharacterManager::Instance().GetInstancePtr(DamageInfoPacket.dwVID);
	bool bSelf = (pInstTarget == CPythonCharacterManager::Instance().GetMainInstancePtr());
	bool bTarget = (pInstTarget == pStream->m_pInstTarget);
	if (pInstTarget)
	{
		if (DamageInfoPacket.damage >= 0)
			pInstTarget->AddDamageEffect(DamageInfoPacket.damage, DamageInfoPacket.flag, bSelf, bTarget);
	}

	return true;
}

bool PhaseGameCombatBridge::HandleDead(CPythonNetworkStream* pStream, const TPacketGCDead& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Network::CombatPacketCodec::DecodeDead(span);
	if (!res.has_value())
		return false;

	const auto& DeadPacket = res.value();

	CPythonCharacterManager& rkChrMgr = CPythonCharacterManager::Instance();
	CInstanceBase* pkChrInstSel = rkChrMgr.GetInstancePtr(DeadPacket.vid);
	if (pkChrInstSel)
	{
		CInstanceBase* pkInstMain = rkChrMgr.GetMainInstancePtr();
		if (pkInstMain == pkChrInstSel)
		{
			Tracenf("주인공 사망");
			if (false == pkInstMain->GetDuelMode())
			{
				PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "OnGameOver", Py_BuildValue("()"));
			}
			CPythonPlayer::Instance().NotifyDeadMainCharacter();
		}

		pkChrInstSel->Die();
	}

	return true;
}
