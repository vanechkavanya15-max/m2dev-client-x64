#pragma once

#ifndef TEST_MODE_DISABLE_STDAFX
#include "StdAfx.h"
#endif

#include "Packet.h"

class CPythonNetworkStream;

class PhaseGamePartyBridge
{
public:
    static bool HandlePartyInvite(CPythonNetworkStream* pStream, const TPacketGCPartyInvite& pack);
    static bool HandlePartyAdd(CPythonNetworkStream* pStream, const TPacketGCPartyAdd& pack);
    static bool HandlePartyUpdate(CPythonNetworkStream* pStream, const TPacketGCPartyUpdate& pack);
    static bool HandlePartyRemove(CPythonNetworkStream* pStream, const TPacketGCPartyRemove& pack);
    static bool HandlePartyParameter(CPythonNetworkStream* pStream, const TPacketGCPartyParameter& pack);
};
