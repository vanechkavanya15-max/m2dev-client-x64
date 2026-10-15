#pragma once

#ifndef TEST_MODE_DISABLE_STDAFX
#include "StdAfx.h"
#endif

#include "Packet.h"

class CPythonNetworkStream;

class PhaseGameChatBridge
{
public:
	static bool HandleChat(CPythonNetworkStream* pStream);
	static bool HandleChat(CPythonNetworkStream* pStream, const TPacketGCChat& kChat, const char* pChatBuf, size_t chatBufSize);
	static bool HandleWhisper(CPythonNetworkStream* pStream);
	static bool HandleWhisper(CPythonNetworkStream* pStream, const TPacketGCWhisper& whisperPacket, const char* pChatBuf, size_t chatBufSize);
	static bool SendChat(CPythonNetworkStream* pStream, const char* c_szChat, BYTE byType = CHAT_TYPE_TALKING);
	static bool SendWhisper(CPythonNetworkStream* pStream, const char* name, const char* c_szChat);
};
