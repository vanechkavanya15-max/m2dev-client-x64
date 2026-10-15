#include "StdAfx.h"
#include "PythonPlayer.h"
#include "PythonItem.h"
#include "GameLib/ItemManager.h"
#include "PythonNetworkStream.h"
#include "Core/EventBus.h"
#include "Core/InventoryEvents.h"
#include "Services/IInventoryService.h"
#include "Services/InventoryService.h"
#include "Client/Bridge/StranglerFacade.h"

bool CPythonPlayer::SendItemMovePacket(TItemPos pos, TItemPos change_pos, BYTE num)
{
	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	return rkNetStream.SendItemMovePacket(pos, change_pos, num);
}

TItemData* CPythonPlayer::GetCompatItemPtr(const TItemPos& Cell) const
{
	switch (Cell.window_type)
	{
	case INVENTORY:
#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
		if (Cell.IsBeltInventoryCell())
		{
			uint16_t beltSlot = Cell.cell - c_Belt_Inventory_Slot_Start;
			if (beltSlot < Client::Gameplay::InventoryDomain::BELT_INVENTORY_MAX_NUM)
				return const_cast<TItemData*>(&m_beltItemDataCompat[beltSlot]);
			return nullptr;
		}
#endif
		if (Cell.IsEquipCell())
		{
			uint16_t eqSlot = Cell.cell - c_Equipment_Start;
			if (eqSlot < Client::Gameplay::InventoryDomain::EQUIPMENT_MAX_NUM)
				return const_cast<TItemData*>(&m_equipItemDataCompat[eqSlot]);
			return nullptr;
		}
		if (Cell.cell < c_Inventory_Count)
			return const_cast<TItemData*>(&m_itemDataCompat[Cell.cell]);
		return nullptr;

	case EQUIPMENT:
	{
		uint16_t actualCell = (Cell.cell >= c_Equipment_Start) ? (Cell.cell - c_Equipment_Start) : Cell.cell;
		if (actualCell < Client::Gameplay::InventoryDomain::EQUIPMENT_MAX_NUM)
			return const_cast<TItemData*>(&m_equipItemDataCompat[actualCell]);
		return nullptr;
	}
#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
	case BELT_INVENTORY:
	{
		uint16_t actualCell = (Cell.cell >= c_Belt_Inventory_Slot_Start) ? (Cell.cell - c_Belt_Inventory_Slot_Start) : Cell.cell;
		if (actualCell < Client::Gameplay::InventoryDomain::BELT_INVENTORY_MAX_NUM)
			return const_cast<TItemData*>(&m_beltItemDataCompat[actualCell]);
		return nullptr;
	}
#endif
	case DRAGON_SOUL_INVENTORY:
		if (Cell.cell < c_DragonSoul_Inventory_Count)
			return const_cast<TItemData*>(&m_dsItemDataCompat[Cell.cell]);
		return nullptr;
	default:
		return nullptr;
	}
}

std::optional<std::pair<Client::Gameplay::InventoryWindow, EterBase::ItemSlot>> CPythonPlayer::MapItemPosToDomain(const TItemPos& Cell) const
{
	if (!Cell.IsValidCell())
		return std::nullopt;

	switch (Cell.window_type)
	{
	case INVENTORY:
#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
		if (Cell.IsBeltInventoryCell())
		{
			uint16_t beltSlot = Cell.cell - c_Belt_Inventory_Slot_Start;
			if (beltSlot < Client::Gameplay::InventoryDomain::BELT_INVENTORY_MAX_NUM)
				return std::make_pair(Client::Gameplay::InventoryWindow::Belt, EterBase::ItemSlot(beltSlot));
			return std::nullopt;
		}
#endif
		if (Cell.IsEquipCell())
		{
			uint16_t eqSlot = Cell.cell - c_Equipment_Start;
			if (eqSlot < Client::Gameplay::InventoryDomain::EQUIPMENT_MAX_NUM)
				return std::make_pair(Client::Gameplay::InventoryWindow::Equipment, EterBase::ItemSlot(eqSlot));
			return std::nullopt;
		}
		if (Cell.cell < Client::Gameplay::InventoryDomain::INVENTORY_MAX_NUM)
		{
			return std::make_pair(Client::Gameplay::InventoryWindow::Inventory, EterBase::ItemSlot(Cell.cell));
		}
		return std::nullopt;

	case EQUIPMENT:
	{
		uint16_t eqSlot = (Cell.cell >= c_Equipment_Start) ? (Cell.cell - c_Equipment_Start) : Cell.cell;
		if (eqSlot < Client::Gameplay::InventoryDomain::EQUIPMENT_MAX_NUM)
			return std::make_pair(Client::Gameplay::InventoryWindow::Equipment, EterBase::ItemSlot(eqSlot));
		return std::nullopt;
	}

	case DRAGON_SOUL_INVENTORY:
		if (Cell.cell < Client::Gameplay::InventoryDomain::DRAGON_SOUL_INVENTORY_MAX_NUM)
			return std::make_pair(Client::Gameplay::InventoryWindow::DragonSoul, EterBase::ItemSlot(Cell.cell));
		return std::nullopt;

#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
	case BELT_INVENTORY:
	{
		uint16_t beltSlot = (Cell.cell >= c_Belt_Inventory_Slot_Start) ? (Cell.cell - c_Belt_Inventory_Slot_Start) : Cell.cell;
		if (beltSlot < Client::Gameplay::InventoryDomain::BELT_INVENTORY_MAX_NUM)
			return std::make_pair(Client::Gameplay::InventoryWindow::Belt, EterBase::ItemSlot(beltSlot));
		return std::nullopt;
	}
#endif

	default:
		return std::nullopt;
	}
}

void CPythonPlayer::UpdateCompatItem(const TItemPos& Cell, const TItemData& item)
{
	TItemData* pCompat = GetCompatItemPtr(Cell);
	if (pCompat)
	{
		*pCompat = item;
	}
}

void CPythonPlayer::MoveItemData(TItemPos SrcCell, TItemPos DstCell)
{
	if (!SrcCell.IsValidCell() || !DstCell.IsValidCell())
		return;

	auto srcMapped = MapItemPosToDomain(SrcCell);
	auto dstMapped = MapItemPosToDomain(DstCell);
	if (srcMapped.has_value() && dstMapped.has_value() && srcMapped->first == dstMapped->first)
	{
		(void)GetInventoryDomain().SwapItem(srcMapped->first, srcMapped->second, dstMapped->second);
	}

	TItemData* pSrcCompat = GetCompatItemPtr(SrcCell);
	TItemData* pDstCompat = GetCompatItemPtr(DstCell);
	if (pSrcCompat && pDstCompat)
	{
		std::swap(*pSrcCompat, *pDstCompat);
	}
	else
	{
		const TItemData* pSrcItem = GetItemData(SrcCell);
		const TItemData* pDstItem = GetItemData(DstCell);
		TItemData src_item = pSrcItem ? *pSrcItem : TItemData{};
		TItemData dst_item = pDstItem ? *pDstItem : TItemData{};

		SetItemData(DstCell, src_item);
		SetItemData(SrcCell, dst_item);
	}
}

const TItemData * CPythonPlayer::GetItemData(TItemPos Cell) const
{
	if (!Cell.IsValidCell())
		return NULL;

	auto mapped = MapItemPosToDomain(Cell);
	if (!mapped.has_value())
		return NULL;

	TItemData* pCompat = GetCompatItemPtr(Cell);
	if (!pCompat)
		return NULL;

	auto itemRes = GetInventoryDomain().GetItem(mapped->first, mapped->second);
	if (itemRes.has_value())
	{
		const auto& item = itemRes.value();
		pCompat->vnum = item.vnum.get();
		pCompat->count = static_cast<uint8_t>(item.count);
		pCompat->flags = item.flags;
		pCompat->anti_flags = item.anti_flags;
		for (size_t i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
		{
			pCompat->alSockets[i] = (i < item.sockets.size()) ? static_cast<int32_t>(item.sockets[i]) : 0;
		}
		for (size_t i = 0; i < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++i)
		{
			if (i < item.attributes.size())
			{
				pCompat->aAttr[i].bType = item.attributes[i].type;
				pCompat->aAttr[i].sValue = item.attributes[i].value;
			}
			else
			{
				pCompat->aAttr[i].bType = 0;
				pCompat->aAttr[i].sValue = 0;
			}
		}
	}
	else
	{
		memset(pCompat, 0, sizeof(TItemData));
	}

	return pCompat;
}

void CPythonPlayer::SetItemData(TItemPos Cell, const TItemData & c_rkItemInst)
{
	if (!Cell.IsValidCell())
		return;

	if (c_rkItemInst.vnum != 0)
	{
		CItemData * pItemData;
		if (!CItemManager::Instance().GetItemDataPointer(c_rkItemInst.vnum, &pItemData))
		{
			TraceError("CPythonPlayer::SetItemData(window_type : %d, dwSlotIndex=%d, itemIndex=%d) - Failed to item data\n", Cell.window_type, Cell.cell, c_rkItemInst.vnum);
			return;
		}
	}

	auto mapped = MapItemPosToDomain(Cell);
	if (mapped.has_value())
	{
		auto [window, slot] = mapped.value();
		if (c_rkItemInst.vnum != 0)
		{
			Client::Gameplay::ItemData domainItem;
			domainItem.vnum = EterBase::ItemVnum(c_rkItemInst.vnum);
			domainItem.count = (c_rkItemInst.count > 0) ? c_rkItemInst.count : 1;
			domainItem.flags = c_rkItemInst.flags;
			domainItem.anti_flags = c_rkItemInst.anti_flags;

			CItemData* pItemData = nullptr;
			if (CItemManager::Instance().GetItemDataPointer(c_rkItemInst.vnum, &pItemData) && pItemData)
			{
				BYTE bHeight = pItemData->GetSize();
				domainItem.size = {1, (bHeight > 0 ? bHeight : static_cast<uint8_t>(1))};
			}
			else
			{
				domainItem.size = {1, 1};
			}

			for (size_t i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM && i < domainItem.sockets.size(); ++i)
			{
				domainItem.sockets[i] = static_cast<uint32_t>(c_rkItemInst.alSockets[i]);
			}
			for (size_t i = 0; i < ITEM_ATTRIBUTE_SLOT_MAX_NUM && i < domainItem.attributes.size(); ++i)
			{
				domainItem.attributes[i].type = c_rkItemInst.aAttr[i].bType;
				domainItem.attributes[i].value = c_rkItemInst.aAttr[i].sValue;
			}

			(void)GetInventoryDomain().RemoveItem(window, slot);
			auto setRes = GetInventoryDomain().SetItem(window, slot, domainItem);
			if (!setRes.has_value())
			{
				TraceError("CPythonPlayer::SetItemData GetInventoryDomain().SetItem error: %d\n", static_cast<int>(setRes.error()));
			}
		}
		else
		{
			(void)GetInventoryDomain().RemoveItem(window, slot);
		}
	}

	UpdateCompatItem(Cell, c_rkItemInst);

	if (Cell.window_type == INVENTORY && c_rkItemInst.vnum != 0)
	{
		UserInterface::Core::EventBus::GetInstance().Publish(
			UserInterface::Core::InventoryEvents::ItemAcquired(
				EterBase::ItemVnum(c_rkItemInst.vnum),
				EterBase::ItemSlot(Cell.cell),
				c_rkItemInst.count
			)
		);
	}

	if (Cell.window_type == INVENTORY)
	{
		EterBase::ItemSlot slot(Cell.cell);
		UserInterface::Services::InventoryItemView itemView{};
		itemView.slot = slot;
		itemView.vnum = EterBase::ItemVnum(c_rkItemInst.vnum);
		itemView.count = c_rkItemInst.count;
		for (size_t i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
		{
			itemView.sockets[i] = c_rkItemInst.alSockets[i];
		}
		for (size_t i = 0; i < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++i)
		{
			itemView.attrTypes[i] = c_rkItemInst.aAttr[i].bType;
			itemView.attrValues[i] = c_rkItemInst.aAttr[i].sValue;
		}

		if (c_rkItemInst.vnum != 0)
		{
			(void)UserInterface::Services::InventoryService::Instance().SetItem(slot, itemView);
		}
		else
		{
			(void)UserInterface::Services::InventoryService::Instance().RemoveItem(slot);
		}
	}
}

DWORD CPythonPlayer::GetItemIndex(TItemPos Cell)
{
	if (!Cell.IsValidCell())
		return 0;

	auto mapped = MapItemPosToDomain(Cell);
	if (!mapped.has_value())
		return 0;

	auto itemRes = GetInventoryDomain().GetItem(mapped->first, mapped->second);
	if (!itemRes.has_value())
		return 0;

	return itemRes->vnum.get();
}

DWORD CPythonPlayer::GetItemFlags(TItemPos Cell)
{
	if (!Cell.IsValidCell())
		return 0;

	auto mapped = MapItemPosToDomain(Cell);
	if (!mapped.has_value())
		return 0;

	auto itemRes = GetInventoryDomain().GetItem(mapped->first, mapped->second);
	if (!itemRes.has_value())
		return 0;

	return itemRes->flags;
}

DWORD CPythonPlayer::GetItemCount(TItemPos Cell)
{
	if (!Cell.IsValidCell())
		return 0;

	auto mapped = MapItemPosToDomain(Cell);
	if (!mapped.has_value())
		return 0;

	auto itemRes = GetInventoryDomain().GetItem(mapped->first, mapped->second);
	if (!itemRes.has_value())
		return 0;

	return itemRes->count;
}

DWORD CPythonPlayer::GetItemCountByVnum(DWORD dwVnum)
{
	DWORD dwCount = 0;
	for (uint16_t i = 0; i < Client::Gameplay::InventoryDomain::INVENTORY_MAX_NUM; ++i)
	{
		auto itemRes = GetInventoryDomain().GetItem(Client::Gameplay::InventoryWindow::Inventory, EterBase::ItemSlot(i));
		if (itemRes.has_value() && itemRes->vnum.get() == dwVnum)
		{
			dwCount += itemRes->count;
		}
	}
	return dwCount;
}

DWORD CPythonPlayer::GetItemMetinSocket(TItemPos Cell, DWORD dwMetinSocketIndex)
{
	if (!Cell.IsValidCell() || dwMetinSocketIndex >= ITEM_SOCKET_SLOT_MAX_NUM)
		return 0;

	auto mapped = MapItemPosToDomain(Cell);
	if (!mapped.has_value())
		return 0;

	auto itemRes = GetInventoryDomain().GetItem(mapped->first, mapped->second);
	if (!itemRes.has_value() || dwMetinSocketIndex >= itemRes->sockets.size())
		return 0;

	return itemRes->sockets[dwMetinSocketIndex];
}

void CPythonPlayer::GetItemAttribute(TItemPos Cell, DWORD dwAttrSlotIndex, BYTE * pbyType, short * psValue)
{
	*pbyType = 0;
	*psValue = 0;

	if (!Cell.IsValidCell() || dwAttrSlotIndex >= ITEM_ATTRIBUTE_SLOT_MAX_NUM)
		return;

	auto mapped = MapItemPosToDomain(Cell);
	if (!mapped.has_value())
		return;

	auto itemRes = GetInventoryDomain().GetItem(mapped->first, mapped->second);
	if (!itemRes.has_value() || dwAttrSlotIndex >= itemRes->attributes.size())
		return;

	*pbyType = itemRes->attributes[dwAttrSlotIndex].type;
	*psValue = itemRes->attributes[dwAttrSlotIndex].value;
}

void CPythonPlayer::SetItemCount(TItemPos Cell, BYTE byCount)
{
	if (!Cell.IsValidCell())
		return;

	auto mapped = MapItemPosToDomain(Cell);
	if (mapped.has_value())
	{
		auto [window, slot] = mapped.value();
		auto itemRes = GetInventoryDomain().GetItem(window, slot);
		if (itemRes.has_value())
		{
			auto item = itemRes.value();
			item.count = byCount;
			(void)GetInventoryDomain().RemoveItem(window, slot);
			(void)GetInventoryDomain().SetItem(window, slot, item);
		}
	}

	TItemData* pCompat = GetCompatItemPtr(Cell);
	if (pCompat)
		pCompat->count = byCount;

	if (Cell.window_type == INVENTORY)
	{
		EterBase::ItemSlot slot(Cell.cell);
		auto optItem = UserInterface::Services::InventoryService::Instance().GetItem(slot);
		if (optItem.has_value())
		{
			optItem->count = byCount;
			(void)UserInterface::Services::InventoryService::Instance().SetItem(slot, *optItem);
		}
	}

	PyCallClassMemberFunc(m_ppyGameWindow, "RefreshInventory", Py_BuildValue("()"));	
}

void CPythonPlayer::SetItemMetinSocket(TItemPos Cell, DWORD dwMetinSocketIndex, DWORD dwMetinNumber)
{
	if (!Cell.IsValidCell() || dwMetinSocketIndex >= ITEM_SOCKET_SLOT_MAX_NUM)
		return;

	auto mapped = MapItemPosToDomain(Cell);
	if (mapped.has_value())
	{
		auto [window, slot] = mapped.value();
		auto itemRes = GetInventoryDomain().GetItem(window, slot);
		if (itemRes.has_value())
		{
			auto item = itemRes.value();
			if (dwMetinSocketIndex < item.sockets.size())
			{
				item.sockets[dwMetinSocketIndex] = dwMetinNumber;
				(void)GetInventoryDomain().RemoveItem(window, slot);
				(void)GetInventoryDomain().SetItem(window, slot, item);
			}
		}
	}

	TItemData* pCompat = GetCompatItemPtr(Cell);
	if (pCompat && dwMetinSocketIndex < ITEM_SOCKET_SLOT_MAX_NUM)
		pCompat->alSockets[dwMetinSocketIndex] = dwMetinNumber;

	if (Cell.window_type == INVENTORY)
	{
		EterBase::ItemSlot slot(Cell.cell);
		auto optItem = UserInterface::Services::InventoryService::Instance().GetItem(slot);
		if (optItem.has_value())
		{
			if (dwMetinSocketIndex < sizeof(optItem->sockets) / sizeof(optItem->sockets[0]))
			{
				optItem->sockets[dwMetinSocketIndex] = dwMetinNumber;
			}
			(void)UserInterface::Services::InventoryService::Instance().SetItem(slot, *optItem);
		}
	}
}

void CPythonPlayer::SetItemAttribute(TItemPos Cell, DWORD dwAttrIndex, BYTE byType, short sValue)
{
	if (!Cell.IsValidCell() || dwAttrIndex >= ITEM_ATTRIBUTE_SLOT_MAX_NUM)
		return;

	auto mapped = MapItemPosToDomain(Cell);
	if (mapped.has_value())
	{
		auto [window, slot] = mapped.value();
		auto itemRes = GetInventoryDomain().GetItem(window, slot);
		if (itemRes.has_value())
		{
			auto item = itemRes.value();
			if (dwAttrIndex < item.attributes.size())
			{
				item.attributes[dwAttrIndex].type = byType;
				item.attributes[dwAttrIndex].value = sValue;
				(void)GetInventoryDomain().RemoveItem(window, slot);
				(void)GetInventoryDomain().SetItem(window, slot, item);
			}
		}
	}

	TItemData* pCompat = GetCompatItemPtr(Cell);
	if (pCompat && dwAttrIndex < ITEM_ATTRIBUTE_SLOT_MAX_NUM)
	{
		pCompat->aAttr[dwAttrIndex].bType = byType;
		pCompat->aAttr[dwAttrIndex].sValue = sValue;
	}

	if (Cell.window_type == INVENTORY)
	{
		EterBase::ItemSlot slot(Cell.cell);
		auto optItem = UserInterface::Services::InventoryService::Instance().GetItem(slot);
		if (optItem.has_value())
		{
			if (dwAttrIndex < sizeof(optItem->attrTypes) / sizeof(optItem->attrTypes[0]))
			{
				optItem->attrTypes[dwAttrIndex] = byType;
				optItem->attrValues[dwAttrIndex] = sValue;
			}
			(void)UserInterface::Services::InventoryService::Instance().SetItem(slot, *optItem);
		}
	}
}

int CPythonPlayer::GetQuickPage()
{
	return m_quickslotManager.GetQuickPage();
}

void CPythonPlayer::SetQuickPage(int nQuickPageIndex)
{
	m_quickslotManager.SetQuickPage(nQuickPageIndex);
}

DWORD CPythonPlayer::LocalQuickSlotIndexToGlobalQuickSlotIndex(DWORD dwLocalSlotIndex)
{
	return m_quickslotManager.LocalQuickSlotIndexToGlobalQuickSlotIndex(dwLocalSlotIndex);
}

void CPythonPlayer::GetGlobalQuickSlotData(DWORD dwGlobalSlotIndex, DWORD* pdwWndType, DWORD* pdwWndItemPos)
{
	m_quickslotManager.GetGlobalQuickSlotData(dwGlobalSlotIndex, pdwWndType, pdwWndItemPos);
}

void CPythonPlayer::GetLocalQuickSlotData(DWORD dwSlotPos, DWORD* pdwWndType, DWORD* pdwWndItemPos)
{
	m_quickslotManager.GetLocalQuickSlotData(dwSlotPos, pdwWndType, pdwWndItemPos);
}

TQuickSlot & CPythonPlayer::__RefLocalQuickSlot(int SlotIndex)
{
	return m_quickslotManager.RefLocalQuickSlot(SlotIndex);
}

TQuickSlot & CPythonPlayer::__RefGlobalQuickSlot(int SlotIndex)
{
	return m_quickslotManager.RefGlobalQuickSlot(SlotIndex);
}

void CPythonPlayer::RemoveQuickSlotByValue(int iType, int iPosition)
{
	m_quickslotManager.RemoveQuickSlotByValue(iType, iPosition);
}

char CPythonPlayer::IsItem(TItemPos Cell)
{
	if (!Cell.IsValidCell())
		return 0;

	auto mapped = MapItemPosToDomain(Cell);
	if (!mapped.has_value())
		return 0;

	auto itemRes = GetInventoryDomain().GetItem(mapped->first, mapped->second);
	return (itemRes.has_value() && itemRes->vnum.get() != 0) ? 1 : 0;
}

void CPythonPlayer::RequestMoveGlobalQuickSlotToLocalQuickSlot(DWORD dwGlobalSrcSlotIndex, DWORD dwLocalDstSlotIndex)
{
	m_quickslotManager.RequestMoveGlobalQuickSlotToLocalQuickSlot(dwGlobalSrcSlotIndex, dwLocalDstSlotIndex);
}

void CPythonPlayer::RequestAddLocalQuickSlot(DWORD dwLocalSlotIndex, DWORD dwWndType, DWORD dwWndItemPos)
{
	m_quickslotManager.RequestAddLocalQuickSlot(dwLocalSlotIndex, dwWndType, dwWndItemPos);
}

void CPythonPlayer::RequestAddToEmptyLocalQuickSlot(DWORD dwWndType, DWORD dwWndItemPos)
{
	m_quickslotManager.RequestAddToEmptyLocalQuickSlot(dwWndType, dwWndItemPos);
}

void CPythonPlayer::RequestDeleteGlobalQuickSlot(DWORD dwGlobalSlotIndex)
{
	m_quickslotManager.RequestDeleteGlobalQuickSlot(dwGlobalSlotIndex);
}

void CPythonPlayer::RequestUseLocalQuickSlot(DWORD dwLocalSlotIndex)
{
	m_quickslotManager.RequestUseLocalQuickSlot(dwLocalSlotIndex);
}

void CPythonPlayer::AddQuickSlot(int QuickSlotIndex, char IconType, char IconPosition)
{
	m_quickslotManager.AddQuickSlot(QuickSlotIndex, IconType, IconPosition);
}

void CPythonPlayer::DeleteQuickSlot(int QuickSlotIndex)
{
	m_quickslotManager.DeleteQuickSlot(QuickSlotIndex);
}

void CPythonPlayer::MoveQuickSlot(int Source, int Target)
{
	m_quickslotManager.MoveQuickSlot(Source, Target);
}

#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
bool CPythonPlayer::IsBeltInventorySlot(TItemPos Cell)
{
	return Cell.IsBeltInventoryCell();
}
#endif

bool CPythonPlayer::IsInventorySlot(TItemPos Cell)
{
	return !Cell.IsEquipCell() && Cell.IsValidCell();
}

bool CPythonPlayer::IsEquipmentSlot(TItemPos Cell)
{
	return Cell.IsEquipCell();
}

bool CPythonPlayer::IsEquipItemInSlot(TItemPos Cell)
{
	if (!Cell.IsEquipCell())
	{
		return false;
	}

	const TItemData * pData = GetItemData(Cell);
	
	if (NULL == pData)
	{
		return false;
	}

	DWORD dwItemIndex = pData->vnum;

	CItemManager::Instance().SelectItemData(dwItemIndex);
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
	{
		TraceError("Failed to find ItemData - CPythonPlayer::IsEquipItem(window_type=%d, iSlotindex=%d)\n", Cell.window_type, Cell.cell);
		return false;
	}

	return pItemData->IsEquipment() ? true : false;
}

void CPythonPlayer::SendClickItemPacket(DWORD dwIID)
{
	if (IsObserverMode())
		return;

	static DWORD s_dwNextTCPTime = 0;

	DWORD dwCurTime=ELTimer_GetMSec();

	if (dwCurTime >= s_dwNextTCPTime)
	{
		s_dwNextTCPTime=dwCurTime + 500;

		const char * c_szOwnerName;
		if (!CPythonItem::Instance().GetOwnership(dwIID, &c_szOwnerName))
			return;

		if (strlen(c_szOwnerName) > 0)
		if (0 != strcmp(c_szOwnerName, GetName()))
		{
			CItemData * pItemData;
			if (!CItemManager::Instance().GetItemDataPointer(CPythonItem::Instance().GetVirtualNumberOfGroundItem(dwIID), &pItemData))
			{
				Tracenf("CPythonPlayer::SendClickItemPacket(dwIID=%d) : Non-exist item.", dwIID);
				return;
			}
			if (!IsPartyMemberByName(c_szOwnerName) || pItemData->IsAntiFlag(CItemData::ITEM_ANTIFLAG_DROP | CItemData::ITEM_ANTIFLAG_GIVE))
			{
				PyCallClassMemberFunc(m_ppyGameWindow, "OnCannotPickItem", Py_BuildValue("()"));
				return;
			}
		}

		CPythonNetworkStream& rkNetStream=CPythonNetworkStream::Instance();
		rkNetStream.SendItemPickUpPacket(dwIID);
	}
}
