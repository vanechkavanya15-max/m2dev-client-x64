#pragma once

#ifndef TEST_MODE_DISABLE_STDAFX
#include "StdAfx.h"
#endif

#include "Packet.h"

class CPythonNetworkStream;

class PhaseGameSyncBridge
{
public:
	// Teleportacja / Warp
	static bool HandleWarp(CPythonNetworkStream* pStream);
	static bool HandleWarp(CPythonNetworkStream* pStream, const TPacketGCWarp& pack);
	static void Warp(CPythonNetworkStream* pStream, LONG lGlobalX, LONG lGlobalY);
	static void ShowMapName(CPythonNetworkStream* pStream, LONG lLocalX, LONG lLocalY);

	// Synchronizacja czasu serwera
	static bool HandleTime(CPythonNetworkStream* pStream);
	static bool HandleTime(CPythonNetworkStream* pStream, const TPacketGCTime& pack);
	static bool HandleTimeStatus(CPythonNetworkStream* pStream);
	static bool HandleTimeStatus(CPythonNetworkStream* pStream, const TPacketGCTime& pack);

	// Ping / Pong
	static bool HandlePing(CPythonNetworkStream* pStream);
	static bool HandlePing(CPythonNetworkStream* pStream, const TPacketGCPing& pack);

	// Zmiana fazy gry
	static void SetGamePhase(CPythonNetworkStream* pStream);
	static void LeaveGamePhase(CPythonNetworkStream* pStream);
};
