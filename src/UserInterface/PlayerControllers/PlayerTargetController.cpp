#include "../StdAfx.h"
#include "PlayerTargetController.h"

#include "../InstanceBase.h"
#include "EterBase/Timer.h"

namespace UserInterface::PlayerControllers
{
    PlayerTargetController::PlayerTargetController(
        Contracts::IActorProvider* pActorProvider,
        Contracts::INetworkService* pNetworkService)
        : m_pActorProvider(pActorProvider)
        , m_pNetworkService(pNetworkService)
        , m_dwTargetVID(0)
        , m_dwSendingTargetVID(0)
        , m_fTargetUpdateTime(0.0f)
        , m_dwTargetEndTime(0)
        , m_dwVIDPicked(0)
        , m_dwIIDPicked(0)
    {
    }

    bool PlayerTargetController::CanChangeTarget() const
    {
        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return false;

        return pkInstMain->CanChangeTarget();
    }

    void PlayerTargetController::SetTarget(DWORD dwVID, BOOL bForceChange)
    {
        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return;

        if (!pkInstMain->CanChangeTarget())
            return;

        DWORD dwCurrentTime = CTimer::Instance().GetCurrentMillisecond();

        if (IsSameTargetVID(dwVID))
        {
            if (dwVID == pkInstMain->GetVirtualID())
            {
                m_dwTargetVID = 0;
                pkInstMain->OnUntargeted();
                pkInstMain->ClearFlyTargetInstance();
                SendTargetPacket(0);
                return;
            }
            m_dwTargetEndTime = dwCurrentTime + 1000;
            return;
        }

        if (bForceChange)
        {
            m_dwTargetEndTime = dwCurrentTime + 2000;
        }
        else
        {
            if (m_dwTargetEndTime > dwCurrentTime)
                return;

            m_dwTargetEndTime = dwCurrentTime + 1000;
        }

        if (IsTarget())
        {
            CInstanceBase* pTargetedInstance = m_pActorProvider ? m_pActorProvider->GetInstance(m_dwTargetVID) : nullptr;
            if (pTargetedInstance)
                pTargetedInstance->OnUntargeted();
        }

        CInstanceBase* pkInstTarget = m_pActorProvider ? m_pActorProvider->GetInstance(dwVID) : nullptr;
        if (pkInstTarget)
        {
            if (pkInstMain->IsTargetableInstance(*pkInstTarget))
            {
                m_dwTargetVID = dwVID;
                pkInstTarget->OnTargeted();
                pkInstMain->SetFlyTargetInstance(*pkInstTarget);
                SendTargetPacket(dwVID);
                return;
            }
        }

        m_dwTargetVID = 0;
        pkInstMain->ClearFlyTargetInstance();
        SendTargetPacket(0);
    }

    void PlayerTargetController::ClearTarget()
    {
        if (!IsTarget())
            return;

        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (pkInstMain)
            pkInstMain->ClearFlyTargetInstance();

        CInstanceBase* pTargetedInstance = m_pActorProvider ? m_pActorProvider->GetInstance(m_dwTargetVID) : nullptr;
        if (pTargetedInstance)
            pTargetedInstance->OnUntargeted();

        m_dwTargetVID = 0;
        SendTargetPacket(0);
    }

    bool PlayerTargetController::ChangeTargetToPickedInstance()
    {
        if (m_dwVIDPicked != 0)
        {
            SetTarget(m_dwVIDPicked);
            return true;
        }

        return false;
    }

    void PlayerTargetController::Update(float fElapsedTime)
    {
        m_fTargetUpdateTime += fElapsedTime;
    }

    bool PlayerTargetController::SendTargetPacket(DWORD dwVID)
    {
        // Ochrona przed floodem pakietow tego samego celu
        if (m_dwSendingTargetVID == dwVID && m_fTargetUpdateTime < 0.2f)
            return false;

        m_dwSendingTargetVID = dwVID;
        m_fTargetUpdateTime = 0.0f;

        if (m_pNetworkService)
            return m_pNetworkService->SendTargetPacket(dwVID);

        return false;
    }
}
