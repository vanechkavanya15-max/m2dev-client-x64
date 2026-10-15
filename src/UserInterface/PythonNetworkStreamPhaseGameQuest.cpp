#include "StdAfx.h"
#include "PythonNetworkStreamPhaseGameQuest.h"
#include "PythonNetworkStream.h"
#include "PythonQuest.h"
#include "PythonEventManager.h"
#include "../Client/Network/QuestPacketCodec.h"
#include "Client/Network/Handlers/QuestDialogDomainHandler.h"
#include <span>
#include <vector>
#include <string>
#include <cstring>

bool PhaseGameQuestBridge::HandleQuestInfo(CPythonNetworkStream* pStream, const TPacketGCQuestInfo& pack)
{
	uint16_t index;
	uint8_t flag;
	if (!Client::Network::QuestPacketCodec::DecodeQuestInfo(pack, index, flag))
		return false;
		
	const uint8_t& c_rFlag = flag;

	enum
	{
		QUEST_PACKET_TYPE_NONE,
		QUEST_PACKET_TYPE_BEGIN,
		QUEST_PACKET_TYPE_UPDATE,
		QUEST_PACKET_TYPE_END,
	};

	uint8_t byQuestPacketType = QUEST_PACKET_TYPE_NONE;

	if (0 != (c_rFlag & QUEST_SEND_IS_BEGIN))
	{
		uint8_t isBegin;
		if (!pStream->Recv(sizeof(isBegin), &isBegin))
			return false;

		if (isBegin)
			byQuestPacketType = QUEST_PACKET_TYPE_BEGIN;
		else
			byQuestPacketType = QUEST_PACKET_TYPE_END;
	}
	else
	{
		byQuestPacketType = QUEST_PACKET_TYPE_UPDATE;
	}

	char szTitle[30 + 1] = "";
	char szClockName[16 + 1] = "";
	int iClockValue = 0;
	char szCounterName[16 + 1] = "";
	int iCounterValue = 0;
	char szIconFileName[24 + 1] = "";

	if (0 != (c_rFlag & QUEST_SEND_TITLE))
	{
		if (!pStream->Recv(sizeof(szTitle), &szTitle))
			return false;
		szTitle[30]='\0';
	}
	if (0 != (c_rFlag & QUEST_SEND_CLOCK_NAME))
	{
		if (!pStream->Recv(sizeof(szClockName), &szClockName))
			return false;
		szClockName[16]='\0';
	}
	if (0 != (c_rFlag & QUEST_SEND_CLOCK_VALUE))
	{
		if (!pStream->Recv(sizeof(iClockValue), &iClockValue))
			return false;
	}
	if (0 != (c_rFlag & QUEST_SEND_COUNTER_NAME))
	{
		if (!pStream->Recv(sizeof(szCounterName), &szCounterName))
			return false;
		szCounterName[16]='\0';
	}
	if (0 != (c_rFlag & QUEST_SEND_COUNTER_VALUE))
	{
		if (!pStream->Recv(sizeof(iCounterValue), &iCounterValue))
			return false;
	}
	if (0 != (c_rFlag & QUEST_SEND_ICON_FILE))
	{
		if (!pStream->Recv(sizeof(szIconFileName), &szIconFileName))
			return false;
		szIconFileName[24]='\0';
	}

	CPythonQuest& rkQuest = CPythonQuest::Instance();

	if (QUEST_PACKET_TYPE_END == byQuestPacketType)
	{
		rkQuest.DeleteQuestInstance(index);
		PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "BINARY_ClearQuest", Py_BuildValue("(i)", index));
	}
	else if (QUEST_PACKET_TYPE_UPDATE == byQuestPacketType)
	{
		if (!rkQuest.IsQuest(index))
		{
			rkQuest.MakeQuest(index);
		}

		if (strlen(szTitle) > 0)
			rkQuest.SetQuestTitle(index, szTitle);
		if (strlen(szClockName) > 0)
			rkQuest.SetQuestClockName(index, szClockName);
		if (strlen(szCounterName) > 0)
			rkQuest.SetQuestCounterName(index, szCounterName);
		if (strlen(szIconFileName) > 0)
			rkQuest.SetQuestIconFileName(index, szIconFileName);

		if (c_rFlag & QUEST_SEND_CLOCK_VALUE)
			rkQuest.SetQuestClockValue(index, iClockValue);
		if (c_rFlag & QUEST_SEND_COUNTER_VALUE)
			rkQuest.SetQuestCounterValue(index, iCounterValue);
	}
	else if (QUEST_PACKET_TYPE_BEGIN == byQuestPacketType)
	{
		CPythonQuest::SQuestInstance QuestInstance;
		QuestInstance.dwIndex = index;
		QuestInstance.strTitle = szTitle;
		QuestInstance.strClockName = szClockName;
		QuestInstance.iClockValue = iClockValue;
		QuestInstance.strCounterName = szCounterName;
		QuestInstance.iCounterValue = iCounterValue;
		QuestInstance.strIconFileName = szIconFileName;
		CPythonQuest::Instance().RegisterQuestInstance(QuestInstance);
	}

	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "RefreshQuest", Py_BuildValue("()"));
	return true;
}

bool PhaseGameQuestBridge::HandleQuestConfirm(CPythonNetworkStream* pStream, const TPacketGCQuestConfirm& pack)
{
	std::string msg;
	int32_t timeout;
	uint32_t requestPID;
	
	if (!Client::Network::QuestPacketCodec::DecodeQuestConfirm(pack, msg, timeout, requestPID))
		return false;
		
	PyObject * poArg = Py_BuildValue("(sii)", msg.c_str(), timeout, requestPID);
	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "BINARY_OnQuestConfirm", poArg);
	return true;
}

bool PhaseGameQuestBridge::HandleScript(CPythonNetworkStream* pStream, const std::string& script)
{
	int iIndex = CPythonEventManager::Instance().RegisterEventSetFromString(script);
	if (-1 != iIndex)
	{
		CPythonEventManager::Instance().SetVisibleLineCount(iIndex, 30);
		pStream->OnScriptEventStart(0, iIndex);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
// CPythonNetworkStream Quest & Script Methods

bool CPythonNetworkStream::RecvQuestInfoPacket()
{
	TPacketGCQuestInfo QuestInfo;

	if (!Peek(sizeof(TPacketGCQuestInfo), &QuestInfo))
	{
		Tracen("Recv Quest Info Packet Error #1");
		return false;
	}

	if (!Peek(QuestInfo.length))
	{
		Tracen("Recv Quest Info Packet Error #2");
		return false;
	}

	Recv(sizeof(TPacketGCQuestInfo));

	return PhaseGameQuestBridge::HandleQuestInfo(this, QuestInfo);
}

bool CPythonNetworkStream::RecvQuestConfirmPacket()
{
	TPacketGCQuestConfirm kQuestConfirmPacket;
	if (!Recv(sizeof(kQuestConfirmPacket), &kQuestConfirmPacket))
	{
		Tracen("RecvQuestConfirmPacket Error");
		return false;
	}

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&kQuestConfirmPacket), sizeof(kQuestConfirmPacket));
	Client::Network::Handlers::QuestDialogDomainHandler::HandleQuestConfirmPacket(span);
	return PhaseGameQuestBridge::HandleQuestConfirm(this, kQuestConfirmPacket);
}

bool CPythonNetworkStream::RecvScriptPacket()
{
	TPacketGCScript ScriptPacket;

	if (!Recv(sizeof(TPacketGCScript), &ScriptPacket))
	{
		TraceError("RecvScriptPacket_RecvError");
		return false;
	}

	if (ScriptPacket.length < sizeof(TPacketGCScript))
	{
		TraceError("RecvScriptPacket_SizeError");
		return false;
	}

	size_t scriptLen = ScriptPacket.length - sizeof(TPacketGCScript);
	std::vector<uint8_t> payloadBuf(ScriptPacket.length);
	std::memcpy(payloadBuf.data(), &ScriptPacket, sizeof(TPacketGCScript));

	if (scriptLen > 0)
	{
		if (!Recv(static_cast<int>(scriptLen), payloadBuf.data() + sizeof(TPacketGCScript)))
			return false;
	}

	Client::Network::Handlers::QuestDialogDomainHandler::HandleScriptPacket(payloadBuf);

	std::string str(reinterpret_cast<const char*>(payloadBuf.data() + sizeof(TPacketGCScript)), scriptLen);
	int iIndex = CPythonEventManager::Instance().RegisterEventSetFromString(str);

	if (-1 != iIndex)
	{
		CPythonEventManager::Instance().SetVisibleLineCount(iIndex, 30);
		CPythonNetworkStream::Instance().OnScriptEventStart(ScriptPacket.skin, iIndex);
	}

	return true;
}

bool CPythonNetworkStream::SendScriptAnswerPacket(int iAnswer)
{
	auto buffer = Client::Network::Handlers::QuestDialogDomainHandler::EncodeScriptAnswer(iAnswer);
	if (!Send(buffer.size(), buffer.data()))
	{
		Tracen("Send Script Answer Packet Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendScriptButtonPacket(unsigned int iIndex)
{
	auto buffer = Client::Network::Handlers::QuestDialogDomainHandler::EncodeScriptButton(iIndex);
	if (!Send(buffer.size(), buffer.data()))
	{
		Tracen("Send Script Button Packet Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendQuestConfirmPacket(BYTE byAnswer, DWORD dwPID)
{
	auto buffer = Client::Network::Handlers::QuestDialogDomainHandler::EncodeQuestConfirm(byAnswer, dwPID);
	if (!Send(buffer.size(), buffer.data()))
	{
		Tracen("SendQuestConfirmPacket Error");
		return false;
	}

	Tracenf(" SendQuestConfirmPacket : %d, %d", byAnswer, dwPID);
	return true;
}

bool CPythonNetworkStream::SendQuestCancelPacket()
{
	auto buffer = Client::Network::Handlers::QuestDialogDomainHandler::EncodeQuestCancel();
	if (!Send(buffer.size(), buffer.data()))
	{
		Tracen("SendQuestCancelPacket Error");
		return false;
	}

	return true;
}
