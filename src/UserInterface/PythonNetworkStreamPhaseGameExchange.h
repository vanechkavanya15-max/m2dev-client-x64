#pragma once

#ifndef TEST_MODE_DISABLE_STDAFX
#include "StdAfx.h"
#endif

#include "Packet.h"

class CPythonNetworkStream;

class PhaseGameExchangeBridge
{
public:
	static bool HandleExchange(CPythonNetworkStream* pStream, const TPacketGCExchange& pack);
	static bool HandleExchangeSub_Start(CPythonNetworkStream* pStream, const TPacketGCExchange& pack);
	static bool HandleExchangeSub_ItemAdd(CPythonNetworkStream* pStream, const TPacketGCExchange& pack);
	static bool HandleExchangeSub_ItemDel(CPythonNetworkStream* pStream, const TPacketGCExchange& pack);
	static bool HandleExchangeSub_ElkAdd(CPythonNetworkStream* pStream, const TPacketGCExchange& pack);
	static bool HandleExchangeSub_Accept(CPythonNetworkStream* pStream, const TPacketGCExchange& pack);
	static bool HandleExchangeSub_End(CPythonNetworkStream* pStream, const TPacketGCExchange& pack);
	static bool HandleExchangeSub_Already(CPythonNetworkStream* pStream, const TPacketGCExchange& pack);
	static bool HandleExchangeSub_LessElk(CPythonNetworkStream* pStream, const TPacketGCExchange& pack);
};
