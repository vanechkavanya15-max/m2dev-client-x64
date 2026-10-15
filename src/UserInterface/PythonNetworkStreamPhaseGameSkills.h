#pragma once

#ifndef TEST_MODE_DISABLE_STDAFX
#include "StdAfx.h"
#include "Packet.h"
#endif

class CPythonNetworkStream;

class PhaseGameSkillsBridge
{
public:
	static bool HandleSkillLevel(class CPythonNetworkStream* pStream, const TPacketGCSkillLevel& pack);
	static bool HandleSkillLevelNew(class CPythonNetworkStream* pStream, const TPacketGCSkillLevelNew& pack);
	static bool HandleSkillCooltimeEnd(class CPythonNetworkStream* pStream, const TPacketGCSkillCoolTimeEnd& pack);
};
