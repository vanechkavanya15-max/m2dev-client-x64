#pragma once

#ifndef TEST_MODE_DISABLE_STDAFX
#include "StdAfx.h"
#endif

#include "Packet.h"

class CPythonNetworkStream;

class PhaseGameCombatBridge
{
public:
	static bool HandleDamageInfo(CPythonNetworkStream* pStream, const TPacketGCDamageInfo& pack);
	static bool HandleDead(CPythonNetworkStream* pStream, const TPacketGCDead& pack);
};
