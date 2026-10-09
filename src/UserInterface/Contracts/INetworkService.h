#pragma once

#ifndef TEST_MODE_DISABLE_STDAFX
#include "../StdAfx.h"
#else
#include <cstdint>
using DWORD = uint32_t;
using BYTE = uint8_t;
using LONG = int32_t;
#endif

namespace UserInterface::Contracts
{
    class INetworkService
    {
    public:
        virtual ~INetworkService() = default;

        // Ruch i synchronizacja
        virtual bool SendCharacterMovePacket(LONG lX, LONG lY, float fRot, DWORD dwTime, BYTE bFunc, BYTE bArg) = 0;
        virtual bool SendSyncPositionPacket(DWORD dwVID, LONG lX, LONG lY) = 0;

        // Walka i cel
        virtual bool SendAttackPacket(DWORD dwVictimVID, BYTE byType) = 0;
        virtual bool SendTargetPacket(DWORD dwVID) = 0;
        virtual bool SendOnClickPacket(DWORD dwVID) = 0;

        // Umiejetnosci
        virtual bool SendUseSkillPacket(DWORD dwSkillIndex, DWORD dwTargetVID) = 0;

        // Przedmioty
        virtual bool SendItemUsePacket(DWORD dwCell) = 0;
        virtual bool SendItemPickUpPacket(DWORD dwIID) = 0;
        virtual bool SendItemDropPacket(DWORD dwCell, DWORD dwCount) = 0;
        virtual bool SendItemMovePacket(DWORD dwSourceCell, DWORD dwTargetCell, DWORD dwCount) = 0;

        // Uniwersalne wysylanie surowych buforow
        virtual bool SendSpecial(const void* pData, int iSize) = 0;
    };
}
