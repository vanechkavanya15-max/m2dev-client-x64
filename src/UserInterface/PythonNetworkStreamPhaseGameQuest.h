#pragma once

#include "PythonNetworkStream.h"
#include <string>

class PhaseGameQuestBridge
{
public:
	static bool HandleQuestInfo(CPythonNetworkStream* pStream, const TPacketGCQuestInfo& pack);
	static bool HandleQuestConfirm(CPythonNetworkStream* pStream, const TPacketGCQuestConfirm& pack);
	static bool HandleScript(CPythonNetworkStream* pStream, const std::string& script);
};
