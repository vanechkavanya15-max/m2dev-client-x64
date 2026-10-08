#include "StdAfx.h"
#include "PythonNetworkStreamPhaseGameShop.h"
#include "PythonNetworkStream.h"
#include "PythonShop.h"
#include "PythonPlayer.h"
#include "Client/Network/ShopPacketCodec.h"
#include <span>
#include <unordered_map>

bool PhaseGameShopBridge::HandleShop(CPythonNetworkStream* pStream, const TPacketGCShop& headerPack, const std::vector<char>& buf)
{
	if (!pStream)
		return false;

	static const std::unordered_map<uint8_t, bool (*)(CPythonNetworkStream*, const std::vector<char>&)> handlers = {
		{ ShopSub::GC::START,               &PhaseGameShopBridge::HandleShopStart },
		{ ShopSub::GC::START_EX,            &PhaseGameShopBridge::HandleShopStartEx },
		{ ShopSub::GC::END,                 &PhaseGameShopBridge::HandleShopEnd },
		{ ShopSub::GC::UPDATE_ITEM,         &PhaseGameShopBridge::HandleShopUpdateItem },
		{ ShopSub::GC::UPDATE_PRICE,        &PhaseGameShopBridge::HandleShopUpdatePrice },
	};

	auto it = handlers.find(headerPack.subheader);
	if (it != handlers.end())
	{
		return it->second(pStream, buf);
	}

	// Bledy i powiadomienia GUI
	switch (headerPack.subheader)
	{
		case ShopSub::GC::NOT_ENOUGH_MONEY:
			PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "OnShopError", Py_BuildValue("(s)", "NOT_ENOUGH_MONEY"));
			return true;
		case ShopSub::GC::NOT_ENOUGH_MONEY_EX:
			PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "OnShopError", Py_BuildValue("(s)", "NOT_ENOUGH_MONEY_EX"));
			return true;
		case ShopSub::GC::SOLDOUT:
			PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "OnShopError", Py_BuildValue("(s)", "SOLDOUT"));
			return true;
		case ShopSub::GC::INVENTORY_FULL:
			PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "OnShopError", Py_BuildValue("(s)", "INVENTORY_FULL"));
			return true;
		case ShopSub::GC::INVALID_POS:
			PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "OnShopError", Py_BuildValue("(s)", "INVALID_POS"));
			return true;
		default:
			TraceError("PhaseGameShopBridge::HandleShop: unknown subheader %d", headerPack.subheader);
			return true;
	}
}

bool PhaseGameShopBridge::HandleShopStart(CPythonNetworkStream* pStream, const std::vector<char>& buf)
{
	if (!pStream || buf.size() < sizeof(DWORD) + sizeof(TPacketGCShopStart))
		return false;

	CPythonShop::Instance().Clear();

	DWORD dwVID = *reinterpret_cast<const DWORD*>(&buf[0]);

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&buf[4]), buf.size() - 4);
	auto res = Client::Network::ShopPacketCodec::DecodeShopStart(span);
	if (!res.has_value())
		return false;

	const auto& pShopStartPacket = res.value();
	for (BYTE iItemIndex = 0; iItemIndex < SHOP_HOST_ITEM_MAX_NUM; ++iItemIndex)
	{
		CPythonShop::Instance().SetItemData(iItemIndex, pShopStartPacket.items[iItemIndex]);
	}

	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "StartShop", Py_BuildValue("(i)", dwVID));
	return true;
}

bool PhaseGameShopBridge::HandleShopStartEx(CPythonNetworkStream* pStream, const std::vector<char>& buf)
{
	if (!pStream || buf.size() < sizeof(TPacketGCShopStartEx))
		return false;

	CPythonShop::Instance().Clear();

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(buf.data()), buf.size());
	auto res = Client::Network::ShopPacketCodec::DecodeShopStartEx(span);
	if (!res.has_value())
		return false;

	const auto& pShopStartPacket = res.value();
	size_t read_point = sizeof(TPacketGCShopStartEx);

	DWORD dwVID = pShopStartPacket.owner_vid;
	BYTE shop_tab_count = pShopStartPacket.shop_tab_count;

	CPythonShop::Instance().SetTabCount(shop_tab_count);

	for (unsigned char i = 0; i < shop_tab_count; i++)
	{
		if (read_point + sizeof(TPacketGCShopStartEx::TSubPacketShopTab) > buf.size())
			break;

		const auto* pPackTab = reinterpret_cast<const TPacketGCShopStartEx::TSubPacketShopTab*>(&buf[read_point]);
		read_point += sizeof(TPacketGCShopStartEx::TSubPacketShopTab);

		CPythonShop::Instance().SetTabCoinType(i, pPackTab->coin_type);
		CPythonShop::Instance().SetTabName(i, pPackTab->name);

		const struct packet_shop_item* item = &pPackTab->items[0];

		for (BYTE j = 0; j < SHOP_HOST_ITEM_MAX_NUM; j++)
		{
			const TShopItemData* itemData = (item + j);
			CPythonShop::Instance().SetItemData(i, j, *itemData);
		}
	}

	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "StartShop", Py_BuildValue("(i)", dwVID));
	return true;
}

bool PhaseGameShopBridge::HandleShopEnd(CPythonNetworkStream* pStream, const std::vector<char>& /*buf*/)
{
	if (!pStream)
		return false;

	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "EndShop", Py_BuildValue("()"));
	return true;
}

bool PhaseGameShopBridge::HandleShopUpdateItem(CPythonNetworkStream* pStream, const std::vector<char>& buf)
{
	if (!pStream || buf.size() < sizeof(TPacketGCShopUpdateItem))
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(buf.data()), buf.size());
	auto res = Client::Network::ShopPacketCodec::DecodeShopUpdateItem(span);
	if (!res.has_value())
		return false;

	const auto& pShopUpdateItemPacket = res.value();
	CPythonShop::Instance().SetItemData(pShopUpdateItemPacket.pos, pShopUpdateItemPacket.item);
	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "RefreshShop", Py_BuildValue("()"));
	return true;
}

bool PhaseGameShopBridge::HandleShopUpdatePrice(CPythonNetworkStream* pStream, const std::vector<char>& buf)
{
	if (!pStream || buf.size() < sizeof(int))
		return false;

	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "SetShopSellingPrice", Py_BuildValue("(i)", *reinterpret_cast<const int*>(&buf[0])));
	return true;
}

bool PhaseGameShopBridge::HandleShopSign(CPythonNetworkStream* pStream, const TPacketGCShopSign& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::ShopPacketCodec::DecodeShopSign(span);
	if (!res.has_value())
		return false;

	const auto& p = res.value();
	CPythonPlayer& rkPlayer = CPythonPlayer::Instance();

	if (0 == strlen(p.szSign))
	{
		PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "BINARY_PrivateShop_Disappear", Py_BuildValue("(i)", p.dwVID));
		if (rkPlayer.IsMainCharacterIndex(p.dwVID))
			rkPlayer.ClosePrivateShop();
	}
	else
	{
		PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "BINARY_PrivateShop_Appear", Py_BuildValue("(is)", p.dwVID, p.szSign));
		if (rkPlayer.IsMainCharacterIndex(p.dwVID))
			rkPlayer.OpenPrivateShop();
	}

	return true;
}
