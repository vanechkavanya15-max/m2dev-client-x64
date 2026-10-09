#include "StdAfx.h"
#include "QuickslotManager.h"
#include "PythonNetworkStream.h"
#include "Client/Bridge/StranglerFacade.h"
#include <algorithm>
#include <cstring>

Client::Gameplay::QuickslotDomain& QuickslotManager::GetDomain() noexcept
{
    return Client::Bridge::StranglerFacade::Instance().GetWorldContext().quickslot;
}

const Client::Gameplay::QuickslotDomain& QuickslotManager::GetDomain() const noexcept
{
    return Client::Bridge::StranglerFacade::Instance().GetWorldContext().quickslot;
}

QuickslotManager::QuickslotManager()
{
    Clear();
}

void QuickslotManager::Clear() noexcept
{
    GetDomain().Clear();
    m_quickSlots.fill(TQuickSlot{0, 0});
}

int QuickslotManager::GetQuickPage() const
{
    return GetDomain().GetPage();
}

void QuickslotManager::SetQuickPage(int nQuickPageIndex)
{
    GetDomain().SetPage(nQuickPageIndex);
    if (m_pageChangeHandler)
    {
        m_pageChangeHandler();
    }
}

DWORD QuickslotManager::LocalQuickSlotIndexToGlobalQuickSlotIndex(DWORD dwLocalSlotIndex) const
{
    return GetDomain().LocalToGlobalIndex(dwLocalSlotIndex);
}

void QuickslotManager::GetGlobalQuickSlotData(DWORD dwGlobalSlotIndex, DWORD* pdwWndType, DWORD* pdwWndItemPos) const
{
    if (!pdwWndType || !pdwWndItemPos)
        return;

    auto slotRes = GetDomain().GetSlot(dwGlobalSlotIndex);
    if (slotRes.has_value())
    {
        *pdwWndType = slotRes->type;
        *pdwWndItemPos = slotRes->position;
    }
    else
    {
        *pdwWndType = 0;
        *pdwWndItemPos = 0;
    }
}

void QuickslotManager::GetLocalQuickSlotData(DWORD dwSlotPos, DWORD* pdwWndType, DWORD* pdwWndItemPos) const
{
    if (!pdwWndType || !pdwWndItemPos)
        return;

    auto slotRes = GetDomain().GetLocalSlot(dwSlotPos);
    if (slotRes.has_value())
    {
        *pdwWndType = slotRes->type;
        *pdwWndItemPos = slotRes->position;
    }
    else
    {
        *pdwWndType = 0;
        *pdwWndItemPos = 0;
    }
}

TQuickSlot & QuickslotManager::RefLocalQuickSlot(int SlotIndex)
{
    return RefGlobalQuickSlot(LocalQuickSlotIndexToGlobalQuickSlotIndex(SlotIndex));
}

TQuickSlot & QuickslotManager::RefGlobalQuickSlot(int SlotIndex)
{
    if (SlotIndex < 0 || SlotIndex >= QUICKSLOT_MAX_NUM)
    {
        static TQuickSlot s_kQuickSlot{0, 0};
        s_kQuickSlot.Type = 0;
        s_kQuickSlot.Position = 0;
        return s_kQuickSlot;
    }

    return m_quickSlots[SlotIndex];
}

void QuickslotManager::AddQuickSlot(int QuickslotIndex, char IconType, char IconPosition)
{
    if (QuickslotIndex < 0 || QuickslotIndex >= QUICKSLOT_MAX_NUM)
        return;

    m_quickSlots[QuickslotIndex].Type = static_cast<uint8_t>(IconType);
    m_quickSlots[QuickslotIndex].Position = static_cast<uint8_t>(IconPosition);
    (void)GetDomain().SetSlot(QuickslotIndex, Client::Gameplay::QuickslotItem{
        static_cast<uint8_t>(IconType),
        static_cast<uint8_t>(IconPosition)
    });
}

void QuickslotManager::DeleteQuickSlot(int QuickslotIndex)
{
    if (QuickslotIndex < 0 || QuickslotIndex >= QUICKSLOT_MAX_NUM)
        return;

    m_quickSlots[QuickslotIndex].Type = 0;
    m_quickSlots[QuickslotIndex].Position = 0;
    (void)GetDomain().ClearSlot(QuickslotIndex);
}

void QuickslotManager::MoveQuickSlot(int Source, int Target)
{
    if (Source < 0 || Source >= QUICKSLOT_MAX_NUM)
        return;
    if (Target < 0 || Target >= QUICKSLOT_MAX_NUM)
        return;

    std::swap(m_quickSlots[Source], m_quickSlots[Target]);
    (void)GetDomain().SwapSlots(Source, Target);
}

void QuickslotManager::RemoveQuickSlotByValue(int iType, int iPosition)
{
    for (BYTE i = 0; i < QUICKSLOT_MAX_NUM; ++i)
    {
        if (iType == m_quickSlots[i].Type && iPosition == m_quickSlots[i].Position)
        {
            CPythonNetworkStream::Instance().SendQuickSlotDelPacket(i);
        }
    }
}

void QuickslotManager::RequestMoveGlobalQuickSlotToLocalQuickSlot(DWORD dwGlobalSrcSlotIndex, DWORD dwLocalDstSlotIndex)
{
    DWORD dwGlobalDstSlotIndex = LocalQuickSlotIndexToGlobalQuickSlotIndex(dwLocalDstSlotIndex);
    CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
    rkNetStream.SendQuickSlotMovePacket(static_cast<BYTE>(dwGlobalSrcSlotIndex), static_cast<BYTE>(dwGlobalDstSlotIndex));
}

void QuickslotManager::RequestAddLocalQuickSlot(DWORD dwLocalSlotIndex, DWORD dwWndType, DWORD dwWndItemPos)
{
    if (dwLocalSlotIndex >= QUICKSLOT_MAX_COUNT_PER_LINE)
        return;

    DWORD dwGlobalSlotIndex = LocalQuickSlotIndexToGlobalQuickSlotIndex(dwLocalSlotIndex);
    CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
    rkNetStream.SendQuickSlotAddPacket(static_cast<BYTE>(dwGlobalSlotIndex), static_cast<BYTE>(dwWndType), static_cast<BYTE>(dwWndItemPos));
}

void QuickslotManager::RequestAddToEmptyLocalQuickSlot(DWORD dwWndType, DWORD dwWndItemPos)
{
    for (int i = 0; i < QUICKSLOT_MAX_COUNT_PER_LINE; ++i)
    {
        TQuickSlot& rkQuickSlot = RefLocalQuickSlot(i);
        if (0 == rkQuickSlot.Type)
        {
            DWORD dwGlobalQuickSlotIndex = LocalQuickSlotIndexToGlobalQuickSlotIndex(i);
            CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
            rkNetStream.SendQuickSlotAddPacket(static_cast<BYTE>(dwGlobalQuickSlotIndex), static_cast<BYTE>(dwWndType), static_cast<BYTE>(dwWndItemPos));
            return;
        }
    }
}

void QuickslotManager::RequestDeleteGlobalQuickSlot(DWORD dwGlobalSlotIndex)
{
    if (dwGlobalSlotIndex >= QUICKSLOT_MAX_COUNT)
        return;

    CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
    rkNetStream.SendQuickSlotDelPacket(static_cast<BYTE>(dwGlobalSlotIndex));
}

void QuickslotManager::RequestUseLocalQuickSlot(DWORD dwLocalSlotIndex)
{
    if (dwLocalSlotIndex >= QUICKSLOT_MAX_COUNT_PER_LINE)
        return;

    DWORD dwRegisteredType = 0;
    DWORD dwRegisteredItemPos = 0;
    GetLocalQuickSlotData(dwLocalSlotIndex, &dwRegisteredType, &dwRegisteredItemPos);

    switch (dwRegisteredType)
    {
    case SLOT_TYPE_INVENTORY:
    {
        CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
        rkNetStream.SendItemUsePacket(TItemPos(INVENTORY, static_cast<WORD>(dwRegisteredItemPos)));
        break;
    }
    case SLOT_TYPE_SKILL:
    {
        if (m_skillClickHandler)
        {
            m_skillClickHandler(dwRegisteredItemPos);
        }
        break;
    }
    case SLOT_TYPE_EMOTION:
    {
        if (m_emotionActHandler)
        {
            m_emotionActHandler(dwRegisteredItemPos);
        }
        break;
    }
    default:
        break;
    }
}
