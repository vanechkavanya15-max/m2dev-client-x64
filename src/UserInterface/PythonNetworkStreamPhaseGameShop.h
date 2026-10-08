#pragma once

#ifndef TEST_MODE_DISABLE_STDAFX
#include "StdAfx.h"
#endif

#include "Packet.h"
#include <vector>

class CPythonNetworkStream;

class PhaseGameShopBridge
{
public:
	static bool HandleShop(CPythonNetworkStream* pStream, const TPacketGCShop& headerPack, const std::vector<char>& buf);
	static bool HandleShopStart(CPythonNetworkStream* pStream, const std::vector<char>& buf);
	static bool HandleShopStartEx(CPythonNetworkStream* pStream, const std::vector<char>& buf);
	static bool HandleShopEnd(CPythonNetworkStream* pStream, const std::vector<char>& buf);
	static bool HandleShopUpdateItem(CPythonNetworkStream* pStream, const std::vector<char>& buf);
	static bool HandleShopUpdatePrice(CPythonNetworkStream* pStream, const std::vector<char>& buf);
	static bool HandleShopSign(CPythonNetworkStream* pStream, const TPacketGCShopSign& pack);
};
