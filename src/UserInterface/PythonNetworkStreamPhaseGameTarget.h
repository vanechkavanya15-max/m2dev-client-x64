#pragma once

#include "Packet.h"

class CPythonNetworkStream;

class PhaseGameTargetBridge
{
public:
	static bool HandleTargetCreate(CPythonNetworkStream* pStream, const TPacketGCTargetCreate& pack);
	static bool HandleTargetUpdate(CPythonNetworkStream* pStream, const TPacketGCTargetUpdate& pack);
	static bool HandleTargetDelete(CPythonNetworkStream* pStream, const TPacketGCTargetDelete& pack);
	static bool HandleCreateFly(CPythonNetworkStream* pStream, const TPacketGCCreateFly& pack);
};
