#include "StdAfx.h"
#include "PythonNetworkStreamPhaseGameParty.h"
#include "PythonNetworkStream.h"
#include "PythonPlayer.h"
#include "PythonCharacterManager.h"
#include "Client/Network/PartyPacketCodec.h"
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
