#include "StdAfx.h"
#include "PythonNetworkStreamPhaseGameRefine.h"
#include "PythonNetworkStream.h"
#include "PythonMessenger.h"
#include "Client/Network/RefinePacketCodec.h"
#include <span>

bool PhaseGameRefineBridge::HandleRefineInformation(CPythonNetworkStream* pStream, const TPacketGCRefineInformation& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::RefinePacketCodec::DecodeRefineInformation(span);
	if (!res.has_value())
		return false;

	const auto& kRefineInfoPacket = res.value();
	const TRefineTable& rkRefineTable = kRefineInfoPacket.refine_table;

	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), 
		"OpenRefineDialog", 
		Py_BuildValue("(iiii)", 
			kRefineInfoPacket.pos, 
			kRefineInfoPacket.refine_table.result_vnum, 
			rkRefineTable.cost, 
			rkRefineTable.prob));

	for (int i = 0; i < rkRefineTable.material_count; ++i)
	{
		PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), 
			"AppendMaterialToRefineDialog", 
			Py_BuildValue("(ii)", rkRefineTable.materials[i].vnum, rkRefineTable.materials[i].count));
	}

#ifdef _DEBUG
	Tracef(" >> RecvRefineInformationPacket(pos=%d, result_vnum=%d, cost=%d, prob=%d)\n",
		kRefineInfoPacket.pos,
		kRefineInfoPacket.refine_table.result_vnum,
		rkRefineTable.cost,
		rkRefineTable.prob);
#endif

	return true;
}

bool PhaseGameRefineBridge::HandleRefineInformationNew(CPythonNetworkStream* pStream, const TPacketGCRefineInformationNew& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::RefinePacketCodec::DecodeRefineInformationNew(span);
	if (!res.has_value())
		return false;

	const auto& kRefineInfoPacket = res.value();
	const TRefineTable& rkRefineTable = kRefineInfoPacket.refine_table;

	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), 
		"OpenRefineDialog", 
		Py_BuildValue("(iiiii)", 
			kRefineInfoPacket.pos, 
			kRefineInfoPacket.refine_table.result_vnum, 
			rkRefineTable.cost, 
			rkRefineTable.prob, 
			kRefineInfoPacket.type)
	);

	for (int i = 0; i < rkRefineTable.material_count; ++i)
	{
		PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), 
			"AppendMaterialToRefineDialog", 
			Py_BuildValue("(ii)", rkRefineTable.materials[i].vnum, rkRefineTable.materials[i].count));
	}

#ifdef _DEBUG
	Tracef(" >> RecvRefineInformationPacketNew(pos=%d, result_vnum=%d, cost=%d, prob=%d, type=%d)\n",
		kRefineInfoPacket.pos,
		kRefineInfoPacket.refine_table.result_vnum,
		rkRefineTable.cost,
		rkRefineTable.prob,
		kRefineInfoPacket.type);
#endif

	return true;
}

bool PhaseGameRefineBridge::HandleLoverInfo(CPythonNetworkStream* pStream, const TPacketGCLoverInfo& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::RefinePacketCodec::DecodeLoverInfo(span);
	if (!res.has_value())
		return false;

	const auto& kLoverInfo = res.value();
	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), 
		"BINARY_LoverInfo", 
		Py_BuildValue("(si)", kLoverInfo.szName, kLoverInfo.byLovePoint));

#ifdef _DEBUG
	Tracef("RECV LOVER INFO : %s, %d\n", kLoverInfo.szName, kLoverInfo.byLovePoint);
#endif

	return true;
}

bool PhaseGameRefineBridge::HandleMessenger(CPythonNetworkStream* pStream, const TPacketGCMessenger& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::RefinePacketCodec::DecodeMessenger(span);
	if (!res.has_value())
		return false;

	const auto& p = res.value();
	int iSize = p.length - sizeof(p);
	char char_name[24 + 1];

	switch (p.subheader)
	{
		case MessengerSub::GC::LIST:
		{
			TPacketGCMessengerListOnline on;
			while (iSize > 0)
			{
				if (!pStream->Recv(sizeof(TPacketGCMessengerListOffline), &on))
					return false;

				if (!pStream->Recv(on.length, char_name))
					return false;

				char_name[on.length] = 0;

				if (on.connected & MESSENGER_CONNECTED_STATE_ONLINE)
					CPythonMessenger::Instance().OnFriendLogin(char_name);
				else
					CPythonMessenger::Instance().OnFriendLogout(char_name);

				iSize -= sizeof(TPacketGCMessengerListOffline);
				iSize -= on.length;
			}
			break;
		}

		case MessengerSub::GC::LOGIN:
		{
			TPacketGCMessengerLogin pLogin;
			if (!pStream->Recv(sizeof(pLogin), &pLogin))
				return false;
			if (!pStream->Recv(pLogin.length, char_name))
				return false;
			char_name[pLogin.length] = 0;
			CPythonMessenger::Instance().OnFriendLogin(char_name);
			pStream->__RefreshTargetBoardByName(char_name);
			break;
		}

		case MessengerSub::GC::LOGOUT:
		{
			TPacketGCMessengerLogout logout;
			if (!pStream->Recv(sizeof(logout), &logout))
				return false;
			if (!pStream->Recv(logout.length, char_name))
				return false;
			char_name[logout.length] = 0;
			CPythonMessenger::Instance().OnFriendLogout(char_name);
			break;
		}

		case MessengerSub::GC::REMOVE_FRIEND:
		{
			BYTE bLength;
			if (!pStream->Recv(sizeof(bLength), &bLength))
				return false;

			if (!pStream->Recv(bLength, char_name))
				return false;

			char_name[bLength] = 0;

			CPythonMessenger::Instance().RemoveFriend(char_name);
			pStream->__RefreshTargetBoardByName(char_name);
			break;
		}

		default:
			TraceError("PhaseGameRefineBridge::HandleMessenger: unknown subheader %d", p.subheader);
			break;
	}

	return true;
}
