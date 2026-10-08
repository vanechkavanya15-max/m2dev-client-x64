#ifndef TEST_MODE_DISABLE_STDAFX
#include "StdAfx.h"
#endif

#include "PythonNetworkStreamPhaseGameSkills.h"
#ifndef TEST_MODE_DISABLE_STDAFX
#include "PythonNetworkStream.h"
#include "PythonPlayer.h"
#include "Client/Network/SkillPacketCodec.h"
#endif
#include <span>

bool PhaseGameSkillsBridge::HandleSkillLevel(class CPythonNetworkStream* pStream, const TPacketGCSkillLevel& pack)
{
	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::SkillPacketCodec::DecodeSkillLevel(span);
	
	if (!res.has_value())
		return false;

	const auto& decodedPack = res.value();
	
	DWORD dwSlotIndex;
	CPythonPlayer& rkPlayer = CPythonPlayer::Instance();

	for (int i = 0; i < SKILL_MAX_NUM; ++i)
	{
		if (rkPlayer.GetSkillSlotIndex(i, &dwSlotIndex))
			rkPlayer.SetSkillLevel(dwSlotIndex, decodedPack.abSkillLevels[i]);
	}

	pStream->__RefreshSkillWindow();
	pStream->__RefreshStatus();

	return true;
}

bool PhaseGameSkillsBridge::HandleSkillLevelNew(class CPythonNetworkStream* pStream, const TPacketGCSkillLevelNew& pack)
{
	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::SkillPacketCodec::DecodeSkillLevelNew(span);
	
	if (!res.has_value())
		return false;

	const auto& decodedPack = res.value();
	
	CPythonPlayer& rkPlayer = CPythonPlayer::Instance();

	rkPlayer.SetSkill(7, 0);
	rkPlayer.SetSkill(8, 0);

	for (int i = 0; i < SKILL_MAX_NUM; ++i)
	{
		const TPlayerSkill& rPlayerSkill = decodedPack.skills[i];

		if (i >= 112 && i <= 115 && rPlayerSkill.bLevel)
			rkPlayer.SetSkill(7, i);

		if (i >= 116 && i <= 119 && rPlayerSkill.bLevel)
			rkPlayer.SetSkill(8, i);

		rkPlayer.SetSkillLevel_(i, rPlayerSkill.bMasterType, rPlayerSkill.bLevel);
	}

	pStream->__RefreshSkillWindow();
	pStream->__RefreshStatus();

	return true;
}

bool PhaseGameSkillsBridge::HandleSkillCooltimeEnd(class CPythonNetworkStream* pStream, const TPacketGCSkillCoolTimeEnd& pack)
{
	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::SkillPacketCodec::DecodeSkillCooltimeEnd(span);
	
	if (!res.has_value())
		return false;

	const auto& decodedPack = res.value();

	CPythonPlayer::Instance().EndSkillCoolTime(decodedPack.bSkill);

	return true;
}
