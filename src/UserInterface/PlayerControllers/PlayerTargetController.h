#pragma once

#include "../Core/EngineForwardDecls.h"
#include "../Contracts/IActorProvider.h"
#include "../Contracts/INetworkService.h"
#include <cstdint>

namespace UserInterface::PlayerControllers
{
    /**
     * @brief Kontroler odpowiedzialny za bezpieczne zaznaczanie celow, obsluge picked oraz flood limiter pakietow.
     * Zgodny z guardrailami: ZERO MUTEXES, NEVER CACHE POINTERS, INETWORKSERVICE.
     */
    class PlayerTargetController
    {
    public:
        PlayerTargetController(
            Contracts::IActorProvider* pActorProvider = nullptr,
            Contracts::INetworkService* pNetworkService = nullptr);
        ~PlayerTargetController() = default;

        // Rejestracja dostawcow uslug
        void SetActorProvider(Contracts::IActorProvider* pActorProvider) { m_pActorProvider = pActorProvider; }
        void SetNetworkService(Contracts::INetworkService* pNetworkService) { m_pNetworkService = pNetworkService; }

        // Zarzadzanie celem
        void SetTarget(DWORD dwVID, BOOL bForceChange = TRUE);
        void ClearTarget();
        DWORD GetTargetVID() const { return m_dwTargetVID; }
        bool IsTarget() const { return 0 != m_dwTargetVID; }
        bool IsSameTargetVID(DWORD dwVID) const { return dwVID == m_dwTargetVID; }
        bool CanChangeTarget() const;
        bool ChangeTargetToPickedInstance();

        // Obsluga wskazanego obiektu (Picked Actor / Picked Item)
        void SetPickedActorID(DWORD dwVID) { m_dwVIDPicked = dwVID; }
        DWORD GetPickedActorID() const { return m_dwVIDPicked; }
        void SetPickedItemID(DWORD dwIID) { m_dwIIDPicked = dwIID; }
        DWORD GetPickedItemID() const { return m_dwIIDPicked; }
        void ClearPicked() { m_dwVIDPicked = 0; m_dwIIDPicked = 0; }

        // Aktualizacja klatki i obsluga limitera floodu pakietow
        void Update(float fElapsedTime);

        // Bezpieczne wysylanie pakietu celu
        bool SendTargetPacket(DWORD dwVID);

    private:
        Contracts::IActorProvider* m_pActorProvider{nullptr};
        Contracts::INetworkService* m_pNetworkService{nullptr};

        DWORD m_dwTargetVID{0};
        DWORD m_dwSendingTargetVID{0};
        float m_fTargetUpdateTime{0.0f};
        DWORD m_dwTargetEndTime{0};

        DWORD m_dwVIDPicked{0};
        DWORD m_dwIIDPicked{0};
    };
}
