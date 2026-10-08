#include "StdAfx.h"
#include "PythonNetworkStreamPhaseGameGuild.h"
#include "PythonNetworkStream.h"
#include "PythonGuild.h"
#include "PythonMessenger.h"
#include "PythonPlayer.h"
#include "PythonApplication.h"
#include "../Client/Network/GuildPacketCodec.h"
#include <unordered_map>

bool PhaseGameGuildBridge::HandleGuild(CPythonNetworkStream* pStream, const uint8_t* pData, size_t size)
{
	auto result = Client::Network::GuildPacketCodec::Decode(std::span<const uint8_t>(pData, size));
	if (!result)
		return false;

	const TPacketGCGuild& pack = result.value();

	static const std::unordered_map<uint8_t, bool (*)(const TPacketGCGuild&)> handlers = {
		{ GuildSub::GC::LOGIN,                  &PhaseGameGuildBridge::HandleGuildSub_Login },
		{ GuildSub::GC::LOGOUT,                 &PhaseGameGuildBridge::HandleGuildSub_Logout },
	};

	auto it = handlers.find(pack.subheader);
	if (it == handlers.end() || it->second == nullptr)
	{
		TraceError("HandleGuild: unknown or unhandled subheader %d", pack.subheader);
		return true;
	}

	return it->second(pack);
}

bool PhaseGameGuildBridge::HandleGuildSub_Login(const TPacketGCGuild& pack)
{
	uint32_t dwPID = 0; // Strangler bridge mock

	CPythonGuild::TGuildMemberData * pGuildMemberData;
	if (CPythonGuild::Instance().GetMemberDataPtrByPID(dwPID, &pGuildMemberData))
		if (0 != pGuildMemberData->strName.compare(CPythonPlayer::Instance().GetName()))
			CPythonMessenger::Instance().LoginGuildMember(pGuildMemberData->strName.c_str());

	return true;
}

bool PhaseGameGuildBridge::HandleGuildSub_Logout(const TPacketGCGuild& pack)
{
	uint32_t dwPID = 0; // Strangler bridge mock

	CPythonGuild::TGuildMemberData * pGuildMemberData;
	if (CPythonGuild::Instance().GetMemberDataPtrByPID(dwPID, &pGuildMemberData))
		if (0 != pGuildMemberData->strName.compare(CPythonPlayer::Instance().GetName()))
			CPythonMessenger::Instance().LogoutGuildMember(pGuildMemberData->strName.c_str());

	return true;
}

bool PhaseGameGuildBridge::HandleGuildSub_Info(const TPacketGCGuildInfo& info)
{
	CPythonGuild::Instance().EnableGuild();
	CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::Instance().GetGuildInfoRef();
	strncpy(rGuildInfo.szGuildName, info.name, GUILD_NAME_MAX_LEN);
	rGuildInfo.szGuildName[GUILD_NAME_MAX_LEN] = '\0';

	rGuildInfo.dwGuildID = info.guild_id;
	rGuildInfo.dwMasterPID = info.master_pid;
	rGuildInfo.dwGuildLevel = info.level;
	rGuildInfo.dwCurrentExperience = info.exp;
	rGuildInfo.dwCurrentMemberCount = info.member_count;
	rGuildInfo.dwMaxMemberCount = info.max_member_count;
	rGuildInfo.dwGuildMoney = info.gold;
	rGuildInfo.bHasLand = info.hasLand;

	// In real use, this calls __RefreshGuildWindowInfoPage() using python integration.
	return true;
}

bool PhaseGameGuildBridge::HandleGuildSub_Member(const TPacketGCGuildSubMember& member)
{
	return true;
}

bool PhaseGameGuildBridge::HandleGuildSub_War(const TPacketGCGuildWar& war)
{
	return true;
}
