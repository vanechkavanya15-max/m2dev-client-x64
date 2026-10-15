#pragma once

#include "UserInterface/Contracts/INetworkService.h"
#include <cstdint>

namespace Network::Adapters
{
    /**
     * @brief Adapter dostarczajacy implementacje kontraktu INetworkService.
     * Deleguje wywolania sieciowe do glownej fasady CPythonNetworkStream.
     * Odcina kontrolery gracza od bezposredniego dolaczania PythonNetworkStream.h.
     */
    class NetworkServiceAdapter : public UserInterface::Contracts::INetworkService
    {
    public:
        NetworkServiceAdapter() noexcept = default;
        ~NetworkServiceAdapter() override = default;

        NetworkServiceAdapter(const NetworkServiceAdapter&) = delete;
        NetworkServiceAdapter& operator=(const NetworkServiceAdapter&) = delete;
        NetworkServiceAdapter(NetworkServiceAdapter&&) noexcept = default;
        NetworkServiceAdapter& operator=(NetworkServiceAdapter&&) noexcept = default;

        // Implementacja metod INetworkService
        bool SendCharacterMovePacket(LONG lX, LONG lY, float fRot, DWORD dwTime, BYTE bFunc, BYTE bArg) override;
        bool SendSyncPositionPacket(DWORD dwVID, LONG lX, LONG lY) override;

        bool SendAttackPacket(DWORD dwVictimVID, BYTE byType) override;
        bool SendTargetPacket(DWORD dwVID) override;
        bool SendOnClickPacket(DWORD dwVID) override;

        bool SendUseSkillPacket(DWORD dwSkillIndex, DWORD dwTargetVID) override;

        bool SendItemUsePacket(DWORD dwCell) override;
        bool SendItemPickUpPacket(DWORD dwIID) override;
        bool SendItemDropPacket(DWORD dwCell, DWORD dwCount) override;
        bool SendItemMovePacket(DWORD dwSourceCell, DWORD dwTargetCell, DWORD dwCount) override;

        bool SendSpecial(const void* pData, int iSize) override;
    };
}
