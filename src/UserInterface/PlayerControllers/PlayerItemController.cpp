#include "../StdAfx.h"
#include "PlayerItemController.h"

#include "../InstanceBase.h"
#include "../PythonItem.h"

namespace UserInterface::PlayerControllers
{
    PlayerItemController::PlayerItemController(
        Contracts::IActorProvider* pActorProvider,
        Contracts::INetworkService* pNetworkService)
        : m_pActorProvider(pActorProvider)
        , m_pNetworkService(pNetworkService)
        , m_fPickupDistance(300.0f)
        , m_dwLastPickItemID(0)
    {
    }

    const PlayerItemController::SAutoPotionInfo& PlayerItemController::GetAutoPotionInfo(int type) const
    {
        if (type < 0 || type >= AUTO_POTION_TYPE_NUM)
            return m_kAutoPotionInfo[0];

        return m_kAutoPotionInfo[type];
    }

    PlayerItemController::SAutoPotionInfo& PlayerItemController::GetAutoPotionInfo(int type)
    {
        if (type < 0 || type >= AUTO_POTION_TYPE_NUM)
            return m_kAutoPotionInfo[0];

        return m_kAutoPotionInfo[type];
    }

    void PlayerItemController::SetAutoPotionInfo(int type, const SAutoPotionInfo& info)
    {
        if (type >= 0 && type < AUTO_POTION_TYPE_NUM)
        {
            m_kAutoPotionInfo[type] = info;
        }
    }

    void PlayerItemController::Update(float fElapsedTime)
    {
        ProcessAutoPotion();
    }

    void PlayerItemController::ProcessAutoPotion()
    {
        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return;

        // Auto Potion HP
        const SAutoPotionInfo& hpInfo = m_kAutoPotionInfo[AUTO_POTION_TYPE_HP];
        if (hpInfo.bActivated && hpInfo.inventorySlotIndex >= 0)
        {
            // Jezeli gracz jest zraniony, wysylamy pakiet uzycia mikstury
            // Sprawdzenie czy gracz nie jest martwy
            if (!pkInstMain->IsDead())
            {
                // Gdy mikstura jest aktywna, wysylamy okresowo pakiet uzycia
                // lub gdy stan zdrowia wymaga regeneracji
            }
        }

        // Auto Potion SP
        const SAutoPotionInfo& spInfo = m_kAutoPotionInfo[AUTO_POTION_TYPE_SP];
        if (spInfo.bActivated && spInfo.inventorySlotIndex >= 0)
        {
            if (!pkInstMain->IsDead())
            {
                // Analogicznie dla SP
            }
        }
    }

    DWORD PlayerItemController::GetPickableDistance() const
    {
        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (pkInstMain)
        {
            if (pkInstMain->IsMountingHorse())
                return 500;
        }

        return 300;
    }

    void PlayerItemController::PickCloseMoney()
    {
        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return;

        TPixelPosition kPPosMain;
        pkInstMain->NEW_GetPixelPosition(&kPPosMain);

        DWORD dwItemID = 0;
        CPythonItem& rkItem = CPythonItem::Instance();
        if (!rkItem.GetCloseMoney(kPPosMain, &dwItemID, GetPickableDistance()))
            return;

        SendItemPickUpPacket(dwItemID);
    }

    void PlayerItemController::PickCloseItem()
    {
        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return;

        TPixelPosition kPPosMain;
        pkInstMain->NEW_GetPixelPosition(&kPPosMain);

        DWORD dwItemID = 0;
        CPythonItem& rkItem = CPythonItem::Instance();
        if (!rkItem.GetCloseItem(kPPosMain, &dwItemID, GetPickableDistance()))
            return;

        if (m_dwLastPickItemID == dwItemID)
            return;

        m_dwLastPickItemID = dwItemID;
        SendItemPickUpPacket(dwItemID);
    }

    bool PlayerItemController::SendItemPickUpPacket(DWORD dwIID)
    {
        if (m_pNetworkService)
            return m_pNetworkService->SendItemPickUpPacket(dwIID);

        return false;
    }

    bool PlayerItemController::SendItemUsePacket(DWORD dwCell)
    {
        if (m_pNetworkService)
            return m_pNetworkService->SendItemUsePacket(dwCell);

        return false;
    }
}
