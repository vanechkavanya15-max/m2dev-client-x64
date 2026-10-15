#include "StdAfx.h"
#include "PythonNetworkStreamPhaseGameParty.h"
#include "PythonNetworkStream.h"
#include "PythonPlayer.h"
#include "PythonCharacterManager.h"
#include "InstanceBase.h"
#include "Client/Network/PartyPacketCodec.h"
#include "Client/Network/Handlers/PartyPacketDomainHandler.h"
#include <span>

bool PhaseGamePartyBridge::HandlePartyInvite(CPythonNetworkStream* pStream, const TPacketGCPartyInvite& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::PartyPacketCodec::DecodePartyInvite(span);
	if (!res.has_value())
		return false;

	const auto& kPartyInvitePacket = res.value();

	CInstanceBase* pInstance = CPythonCharacterManager::Instance().GetInstancePtr(kPartyInvitePacket.leader_pid);
	if (!pInstance)
	{
		TraceError(" CPythonNetworkStream::RecvPartyInvite - Failed to find leader instance [%d]\n", kPartyInvitePacket.leader_pid);
		return true;
	}

	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "RecvPartyInviteQuestion", Py_BuildValue("(is)", kPartyInvitePacket.leader_pid, pInstance->GetNameString()));
	Tracef(" >> RecvPartyInvite : %d, %s\n", kPartyInvitePacket.leader_pid, pInstance->GetNameString());

	return true;
}

bool PhaseGamePartyBridge::HandlePartyAdd(CPythonNetworkStream* pStream, const TPacketGCPartyAdd& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::PartyPacketCodec::DecodePartyAdd(span);
	if (!res.has_value())
		return false;

	const auto& kPartyAddPacket = res.value();

	CPythonPlayer::Instance().AppendPartyMember(kPartyAddPacket.pid, kPartyAddPacket.name);
	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "AddPartyMember", Py_BuildValue("(is)", kPartyAddPacket.pid, kPartyAddPacket.name));
	Tracef(" >> RecvPartyAdd : %d, %s\n", kPartyAddPacket.pid, kPartyAddPacket.name);

	return true;
}

bool PhaseGamePartyBridge::HandlePartyUpdate(CPythonNetworkStream* pStream, const TPacketGCPartyUpdate& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::PartyPacketCodec::DecodePartyUpdate(span);
	if (!res.has_value())
		return false;

	const auto& kPartyUpdatePacket = res.value();

	CPythonPlayer::TPartyMemberInfo* pPartyMemberInfo;
	if (!CPythonPlayer::Instance().GetPartyMemberPtr(kPartyUpdatePacket.pid, &pPartyMemberInfo))
		return true;

	BYTE byOldState = pPartyMemberInfo->byState;

	CPythonPlayer::Instance().UpdatePartyMemberInfo(kPartyUpdatePacket.pid, kPartyUpdatePacket.state, kPartyUpdatePacket.percent_hp);
	for (int i = 0; i < PARTY_AFFECT_SLOT_MAX_NUM; ++i)
	{
		CPythonPlayer::Instance().UpdatePartyMemberAffect(kPartyUpdatePacket.pid, i, kPartyUpdatePacket.affects[i]);
	}

	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "UpdatePartyMemberInfo", Py_BuildValue("(i)", kPartyUpdatePacket.pid));

	// Jesli lider sie zmienil, zaktualizuj przycisk TargetBoard
	DWORD dwVID;
	if (CPythonPlayer::Instance().PartyMemberPIDToVID(kPartyUpdatePacket.pid, &dwVID))
	{
		if (byOldState != kPartyUpdatePacket.state)
		{
			pStream->__RefreshTargetBoardByVID(dwVID);
		}
	}

	return true;
}

bool PhaseGamePartyBridge::HandlePartyRemove(CPythonNetworkStream* pStream, const TPacketGCPartyRemove& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::PartyPacketCodec::DecodePartyRemove(span);
	if (!res.has_value())
		return false;

	const auto& kPartyRemovePacket = res.value();

	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "RemovePartyMember", Py_BuildValue("(i)", kPartyRemovePacket.pid));
	Tracef(" >> RecvPartyRemove : %d\n", kPartyRemovePacket.pid);

	return true;
}

bool PhaseGamePartyBridge::HandlePartyParameter(CPythonNetworkStream* pStream, const TPacketGCPartyParameter& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::PartyPacketCodec::DecodePartyParameter(span);
	if (!res.has_value())
		return false;

	const auto& kPartyParameterPacket = res.value();

	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "ChangePartyParameter", Py_BuildValue("(i)", kPartyParameterPacket.bDistributeMode));
	Tracef(" >> RecvPartyParameter : %d\n", kPartyParameterPacket.bDistributeMode);

	return true;
}

//////////////////////////////////////////////////////////////////////////
// CPythonNetworkStream Party Methods

bool CPythonNetworkStream::SendPartyInvitePacket(DWORD dwVID)
{
	auto buffer = Client::Network::Handlers::PartyPacketDomainHandler::EncodePartyInvite(dwVID);
	if (!Send(buffer.size(), buffer.data()))
	{
		Tracenf("CPythonNetworkStream::SendPartyInvitePacket [%ud] - PACKET SEND ERROR", dwVID);
		return false;
	}

	Tracef(" << SendPartyInvitePacket : %d\n", dwVID);
	return true;
}

bool CPythonNetworkStream::SendPartyInviteAnswerPacket(DWORD dwLeaderVID, BYTE byAnswer)
{
	auto buffer = Client::Network::Handlers::PartyPacketDomainHandler::EncodePartyInviteAnswer(dwLeaderVID, byAnswer);
	if (!Send(buffer.size(), buffer.data()))
	{
		Tracenf("CPythonNetworkStream::SendPartyInviteAnswerPacket [%ud %ud] - PACKET SEND ERROR", dwLeaderVID, byAnswer);
		return false;
	}

	Tracef(" << SendPartyInviteAnswerPacket : %d, %d\n", dwLeaderVID, byAnswer);
	return true;
}

bool CPythonNetworkStream::SendPartyRemovePacket(DWORD dwPID)
{
	auto buffer = Client::Network::Handlers::PartyPacketDomainHandler::EncodePartyRemove(dwPID);
	if (!Send(buffer.size(), buffer.data()))
	{
		Tracenf("CPythonNetworkStream::SendPartyRemovePacket [%ud] - PACKET SEND ERROR", dwPID);
		return false;
	}

	Tracef(" << SendPartyRemovePacket : %d\n", dwPID);
	return true;
}

bool CPythonNetworkStream::SendPartySetStatePacket(DWORD dwVID, BYTE byState, BYTE byFlag)
{
	auto buffer = Client::Network::Handlers::PartyPacketDomainHandler::EncodePartySetState(dwVID, byState, byFlag);
	if (!Send(buffer.size(), buffer.data()))
	{
		Tracenf("CPythonNetworkStream::SendPartySetStatePacket(%ud, %ud) - PACKET SEND ERROR", dwVID, byState);
		return false;
	}

	Tracef(" << SendPartySetStatePacket : %d, %d, %d\n", dwVID, byState, byFlag);
	return true;
}

bool CPythonNetworkStream::SendPartyUseSkillPacket(BYTE bySkillIndex, DWORD dwVID)
{
	auto buffer = Client::Network::Handlers::PartyPacketDomainHandler::EncodePartyUseSkill(bySkillIndex, dwVID);
	if (!Send(buffer.size(), buffer.data()))
	{
		Tracenf("CPythonNetworkStream::SendPartyUseSkillPacket(%ud, %ud) - PACKET SEND ERROR", bySkillIndex, dwVID);
		return false;
	}

	Tracef(" << SendPartyUseSkillPacket : %d, %d\n", bySkillIndex, dwVID);
	return true;
}

bool CPythonNetworkStream::SendPartyParameterPacket(BYTE byDistributeMode)
{
	auto buffer = Client::Network::Handlers::PartyPacketDomainHandler::EncodePartyParameter(byDistributeMode);
	if (!Send(buffer.size(), buffer.data()))
	{
		Tracenf("CPythonNetworkStream::SendPartyParameterPacket(%d) - PACKET SEND ERROR", byDistributeMode);
		return false;
	}

	Tracef(" << SendPartyParameterPacket : %d\n", byDistributeMode);
	return true;
}

bool CPythonNetworkStream::RecvPartyInvite()
{
	TPacketGCPartyInvite kPartyInvitePacket;
	if (!Recv(sizeof(kPartyInvitePacket), &kPartyInvitePacket))
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&kPartyInvitePacket), sizeof(kPartyInvitePacket));
	Client::Network::Handlers::PartyPacketDomainHandler::HandlePartyInvite(span);
	return PhaseGamePartyBridge::HandlePartyInvite(this, kPartyInvitePacket);
}

bool CPythonNetworkStream::RecvPartyAdd()
{
	TPacketGCPartyAdd kPartyAddPacket;
	if (!Recv(sizeof(kPartyAddPacket), &kPartyAddPacket))
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&kPartyAddPacket), sizeof(kPartyAddPacket));
	Client::Network::Handlers::PartyPacketDomainHandler::HandlePartyAdd(span);
	return PhaseGamePartyBridge::HandlePartyAdd(this, kPartyAddPacket);
}

bool CPythonNetworkStream::RecvPartyUpdate()
{
	TPacketGCPartyUpdate kPartyUpdatePacket;
	if (!Recv(sizeof(kPartyUpdatePacket), &kPartyUpdatePacket))
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&kPartyUpdatePacket), sizeof(kPartyUpdatePacket));
	Client::Network::Handlers::PartyPacketDomainHandler::HandlePartyUpdate(span);
	return PhaseGamePartyBridge::HandlePartyUpdate(this, kPartyUpdatePacket);
}

bool CPythonNetworkStream::RecvPartyRemove()
{
	TPacketGCPartyRemove kPartyRemovePacket;
	if (!Recv(sizeof(kPartyRemovePacket), &kPartyRemovePacket))
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&kPartyRemovePacket), sizeof(kPartyRemovePacket));
	Client::Network::Handlers::PartyPacketDomainHandler::HandlePartyRemove(span);
	return PhaseGamePartyBridge::HandlePartyRemove(this, kPartyRemovePacket);
}

bool CPythonNetworkStream::RecvPartyLink()
{
	TPacketGCPartyLink kPartyLinkPacket;
	if (!Recv(sizeof(kPartyLinkPacket), &kPartyLinkPacket))
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&kPartyLinkPacket), sizeof(kPartyLinkPacket));
	Client::Network::Handlers::PartyPacketDomainHandler::HandlePartyLink(span);

	CPythonPlayer::Instance().LinkPartyMember(kPartyLinkPacket.pid, kPartyLinkPacket.vid);
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "LinkPartyMember", Py_BuildValue("(ii)", kPartyLinkPacket.pid, kPartyLinkPacket.vid));
	Tracef(" >> RecvPartyLink : %d, %d\n", kPartyLinkPacket.pid, kPartyLinkPacket.vid);

	return true;
}

bool CPythonNetworkStream::RecvPartyUnlink()
{
	TPacketGCPartyUnlink kPartyUnlinkPacket;
	if (!Recv(sizeof(kPartyUnlinkPacket), &kPartyUnlinkPacket))
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&kPartyUnlinkPacket), sizeof(kPartyUnlinkPacket));
	Client::Network::Handlers::PartyPacketDomainHandler::HandlePartyUnlink(span);

	CPythonPlayer::Instance().UnlinkPartyMember(kPartyUnlinkPacket.pid);

	if (CPythonPlayer::Instance().IsMainCharacterIndex(kPartyUnlinkPacket.vid))
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "UnlinkAllPartyMember", Py_BuildValue("()"));
	}
	else
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "UnlinkPartyMember", Py_BuildValue("(i)", kPartyUnlinkPacket.pid));
	}

	Tracef(" >> RecvPartyUnlink : %d, %d\n", kPartyUnlinkPacket.pid, kPartyUnlinkPacket.vid);

	return true;
}

bool CPythonNetworkStream::RecvPartyParameter()
{
	TPacketGCPartyParameter kPartyParameterPacket;
	if (!Recv(sizeof(kPartyParameterPacket), &kPartyParameterPacket))
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&kPartyParameterPacket), sizeof(kPartyParameterPacket));
	Client::Network::Handlers::PartyPacketDomainHandler::HandlePartyParameter(span);
	return PhaseGamePartyBridge::HandlePartyParameter(this, kPartyParameterPacket);
}
