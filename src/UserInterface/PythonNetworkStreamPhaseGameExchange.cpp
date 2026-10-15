#include "StdAfx.h"
#include "PythonNetworkStreamPhaseGameExchange.h"
#include "PythonNetworkStream.h"
#include "PythonExchange.h"
#include "PythonPlayer.h"
#include "PythonCharacterManager.h"
#include "Client/Network/ExchangePacketCodec.h"
#include <span>
#include <unordered_map>

bool PhaseGameExchangeBridge::HandleExchange(CPythonNetworkStream* pStream, const TPacketGCExchange& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::ExchangePacketCodec::DecodeExchangePacket(span);
	if (!res.has_value())
		return false;

	const auto& exchange_packet = res.value();

	static const std::unordered_map<uint8_t, bool (*)(CPythonNetworkStream*, const TPacketGCExchange&)> handlers = {
		{ ExchangeSub::GC::START,    &PhaseGameExchangeBridge::HandleExchangeSub_Start },
		{ ExchangeSub::GC::ITEM_ADD, &PhaseGameExchangeBridge::HandleExchangeSub_ItemAdd },
		{ ExchangeSub::GC::ITEM_DEL, &PhaseGameExchangeBridge::HandleExchangeSub_ItemDel },
		{ ExchangeSub::GC::ELK_ADD,  &PhaseGameExchangeBridge::HandleExchangeSub_ElkAdd },
		{ ExchangeSub::GC::ACCEPT,   &PhaseGameExchangeBridge::HandleExchangeSub_Accept },
		{ ExchangeSub::GC::END,      &PhaseGameExchangeBridge::HandleExchangeSub_End },
		{ ExchangeSub::GC::ALREADY,  &PhaseGameExchangeBridge::HandleExchangeSub_Already },
		{ ExchangeSub::GC::LESS_ELK, &PhaseGameExchangeBridge::HandleExchangeSub_LessElk },
	};

	auto it = handlers.find(exchange_packet.subheader);
	if (it == handlers.end())
	{
		TraceError("PhaseGameExchangeBridge::HandleExchange: unknown subheader %d", exchange_packet.subheader);
		return true;
	}

	return it->second(pStream, exchange_packet);
}

bool PhaseGameExchangeBridge::HandleExchangeSub_Start(CPythonNetworkStream* pStream, const TPacketGCExchange& pack)
{
	if (!pStream)
		return false;

	CPythonExchange::Instance().Clear();
	CPythonExchange::Instance().Start();
	CPythonExchange::Instance().SetSelfName(CPythonPlayer::Instance().GetName());

	{
		CInstanceBase* pCharacterInstance = CPythonCharacterManager::Instance().GetInstancePtr(pack.arg1);
		if (pCharacterInstance)
			CPythonExchange::Instance().SetTargetName(pCharacterInstance->GetNameString());
	}

	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "StartExchange", Py_BuildValue("()"));
	return true;
}

bool PhaseGameExchangeBridge::HandleExchangeSub_ItemAdd(CPythonNetworkStream* pStream, const TPacketGCExchange& pack)
{
	if (!pStream)
		return false;

	if (pack.is_me)
	{
		int iSlotIndex = pack.arg2.cell;
		CPythonExchange::Instance().SetItemToSelf(iSlotIndex, pack.arg1, static_cast<BYTE>(pack.arg3));
		for (int i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
			CPythonExchange::Instance().SetItemMetinSocketToSelf(iSlotIndex, i, pack.alValues[i]);
		for (int j = 0; j < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++j)
			CPythonExchange::Instance().SetItemAttributeToSelf(iSlotIndex, j, pack.aAttr[j].bType, pack.aAttr[j].sValue);
	}
	else
	{
		int iSlotIndex = pack.arg2.cell;
		CPythonExchange::Instance().SetItemToTarget(iSlotIndex, pack.arg1, static_cast<BYTE>(pack.arg3));
		for (int i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
			CPythonExchange::Instance().SetItemMetinSocketToTarget(iSlotIndex, i, pack.alValues[i]);
		for (int j = 0; j < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++j)
			CPythonExchange::Instance().SetItemAttributeToTarget(iSlotIndex, j, pack.aAttr[j].bType, pack.aAttr[j].sValue);
	}

	pStream->__RefreshExchangeWindow();
	pStream->__RefreshInventoryWindow();
	return true;
}

bool PhaseGameExchangeBridge::HandleExchangeSub_ItemDel(CPythonNetworkStream* pStream, const TPacketGCExchange& pack)
{
	if (!pStream)
		return false;

	if (pack.is_me)
	{
		CPythonExchange::Instance().DelItemOfSelf(static_cast<BYTE>(pack.arg1));
	}
	else
	{
		CPythonExchange::Instance().DelItemOfTarget(static_cast<BYTE>(pack.arg1));
	}

	pStream->__RefreshExchangeWindow();
	pStream->__RefreshInventoryWindow();
	return true;
}

bool PhaseGameExchangeBridge::HandleExchangeSub_ElkAdd(CPythonNetworkStream* pStream, const TPacketGCExchange& pack)
{
	if (!pStream)
		return false;

	if (pack.is_me)
		CPythonExchange::Instance().SetElkToSelf(pack.arg1);
	else
		CPythonExchange::Instance().SetElkToTarget(pack.arg1);

	pStream->__RefreshExchangeWindow();
	return true;
}

bool PhaseGameExchangeBridge::HandleExchangeSub_Accept(CPythonNetworkStream* pStream, const TPacketGCExchange& pack)
{
	if (!pStream)
		return false;

	if (pack.is_me)
	{
		CPythonExchange::Instance().SetAcceptToSelf(static_cast<BYTE>(pack.arg1));
	}
	else
	{
		CPythonExchange::Instance().SetAcceptToTarget(static_cast<BYTE>(pack.arg1));
	}

	pStream->__RefreshExchangeWindow();
	return true;
}

bool PhaseGameExchangeBridge::HandleExchangeSub_End(CPythonNetworkStream* pStream, const TPacketGCExchange& /*pack*/)
{
	if (!pStream)
		return false;

	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "EndExchange", Py_BuildValue("()"));
	pStream->__RefreshInventoryWindow();
	CPythonExchange::Instance().End();
	return true;
}

bool PhaseGameExchangeBridge::HandleExchangeSub_Already(CPythonNetworkStream* /*pStream*/, const TPacketGCExchange& /*pack*/)
{
	Tracef("trade_already");
	return true;
}

bool PhaseGameExchangeBridge::HandleExchangeSub_LessElk(CPythonNetworkStream* /*pStream*/, const TPacketGCExchange& /*pack*/)
{
	Tracef("trade_less_elk");
	return true;
}
