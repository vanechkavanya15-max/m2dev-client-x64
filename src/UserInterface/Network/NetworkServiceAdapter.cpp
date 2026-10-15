#include "StdAfx.h"
#include "NetworkServiceAdapter.h"
#include "PythonNetworkStream.h"
#include "GameType.h"

namespace Network::Adapters
{
    bool NetworkServiceAdapter::SendCharacterMovePacket(LONG lX, LONG lY, float fRot, DWORD dwTime, BYTE bFunc, BYTE bArg)
    {
        CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
        TPixelPosition pos(static_cast<float>(lX), static_cast<float>(lY), 0.0f);
        return rkNetStream.SendCharacterStatePacket(pos, fRot, bFunc, bArg);
    }

    bool NetworkServiceAdapter::SendSyncPositionPacket(DWORD dwVID, LONG lX, LONG lY)
    {
        CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
        return rkNetStream.SendSyncPositionElementPacket(dwVID, static_cast<DWORD>(lX), static_cast<DWORD>(lY));
    }

    bool NetworkServiceAdapter::SendAttackPacket(DWORD dwVictimVID, BYTE byType)
    {
        CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
        return rkNetStream.SendAttackPacket(static_cast<UINT>(byType), dwVictimVID);
    }

    bool NetworkServiceAdapter::SendTargetPacket(DWORD dwVID)
    {
        CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
        return rkNetStream.SendTargetPacket(dwVID);
    }

    bool NetworkServiceAdapter::SendOnClickPacket(DWORD dwVID)
    {
        CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
        return rkNetStream.SendOnClickPacket(dwVID);
    }

    bool NetworkServiceAdapter::SendUseSkillPacket(DWORD dwSkillIndex, DWORD dwTargetVID)
    {
        CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
        return rkNetStream.SendUseSkillPacket(dwSkillIndex, dwTargetVID);
    }

    bool NetworkServiceAdapter::SendItemUsePacket(DWORD dwCell)
    {
        CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
        TItemPos itemPos(INVENTORY, static_cast<WORD>(dwCell));
        return rkNetStream.SendItemUsePacket(itemPos);
    }

    bool NetworkServiceAdapter::SendItemPickUpPacket(DWORD dwIID)
    {
        CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
        return rkNetStream.SendItemPickUpPacket(dwIID);
    }

    bool NetworkServiceAdapter::SendItemDropPacket(DWORD dwCell, DWORD dwCount)
    {
        CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
        TItemPos itemPos(INVENTORY, static_cast<WORD>(dwCell));
        return rkNetStream.SendItemDropPacketNew(itemPos, 0, dwCount);
    }

    bool NetworkServiceAdapter::SendItemMovePacket(DWORD dwSourceCell, DWORD dwTargetCell, DWORD dwCount)
    {
        CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
        TItemPos srcPos(INVENTORY, static_cast<WORD>(dwSourceCell));
        TItemPos dstPos(INVENTORY, static_cast<WORD>(dwTargetCell));
        return rkNetStream.SendItemMovePacket(srcPos, dstPos, static_cast<BYTE>(dwCount));
    }

    bool NetworkServiceAdapter::SendSpecial(const void* pData, int iSize)
    {
        CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
        return rkNetStream.Send(iSize, pData);
    }
}
