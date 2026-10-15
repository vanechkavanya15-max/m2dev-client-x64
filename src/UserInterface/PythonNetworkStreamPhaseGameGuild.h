#pragma once

#include <cstdint>
#include <cstddef>
#include "Packet.h"

class CPythonNetworkStream;

class PhaseGameGuildBridge
{
public:
	static bool HandleGuild(CPythonNetworkStream* pStream, const uint8_t* pData, size_t size);
	static bool HandleGuildSub_Login(const TPacketGCGuild& pack);
	static bool HandleGuildSub_Logout(const TPacketGCGuild& pack);
	static bool HandleGuildSub_Info(const TPacketGCGuildInfo& info);
	static bool HandleGuildSub_Member(const TPacketGCGuildSubMember& member);
	static bool HandleGuildSub_War(const TPacketGCGuildWar& war);
};
