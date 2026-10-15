#pragma once

#ifndef TEST_MODE_DISABLE_STDAFX
#include "StdAfx.h"
#endif

#include "Packet.h"

class CPythonNetworkStream;

class PhaseGameRefineBridge
{
public:
	static bool HandleRefineInformation(CPythonNetworkStream* pStream, const TPacketGCRefineInformation& pack);
	static bool HandleRefineInformationNew(CPythonNetworkStream* pStream, const TPacketGCRefineInformationNew& pack);
	static bool HandleLoverInfo(CPythonNetworkStream* pStream, const TPacketGCLoverInfo& pack);
	static bool HandleMessenger(CPythonNetworkStream* pStream, const TPacketGCMessenger& pack);
};
