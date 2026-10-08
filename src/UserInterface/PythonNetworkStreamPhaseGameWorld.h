#pragma once

#ifndef TEST_MODE_DISABLE_STDAFX
#include "StdAfx.h"
#endif

#include "Packet.h"

class CPythonNetworkStream;

class PhaseGameWorldBridge
{
public:
	static bool HandleTime(CPythonNetworkStream* pStream, const TPacketGCTime& pack);
	static bool HandleDungeon(CPythonNetworkStream* pStream, const TPacketGCDungeon& pack);
	static bool HandleFishing(CPythonNetworkStream* pStream, const TPacketGCFishing& pack);
	static bool HandleChannel(CPythonNetworkStream* pStream, const TPacketGCChannel& pack);
};
