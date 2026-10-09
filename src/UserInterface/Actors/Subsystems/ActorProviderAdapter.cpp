#include "StdAfx.h"
#include "ActorProviderAdapter.h"
#include "ActorRegistry.h"
#include "CharacterPicker.h"
#include "InstanceBase.h"
#include "PythonCharacterManager.h"
#include "AbstractPlayer.h"
#include "EterBase/LogModern.h"

#include <cmath>

namespace UserInterface::Actors::Subsystems
{
    ActorProviderAdapter::ActorProviderAdapter(const ActorRegistry* pRegistry, const CharacterPicker* pPicker) noexcept
        : m_pRegistry(pRegistry)
        , m_pPicker(pPicker)
    {
    }

    CInstanceBase* ActorProviderAdapter::GetInstance(uint32_t dwVID) const
    {
        if (dwVID == 0)
            return nullptr;

        if (m_pRegistry != nullptr)
        {
            return m_pRegistry->FindActor(dwVID);
        }

        // Delegacja awaryjna do istniejacej fasady CPythonCharacterManager
        return CPythonCharacterManager::Instance().GetInstancePtr(dwVID);
    }

    CInstanceBase* ActorProviderAdapter::GetMainActor() const
    {
        if (m_mainActorVID != 0)
        {
            CInstanceBase* pMain = GetInstance(m_mainActorVID);
            if (pMain != nullptr)
                return pMain;
        }

        // Delegacja awaryjna
        return CPythonCharacterManager::Instance().GetMainActorPtr();
    }

    CInstanceBase* ActorProviderAdapter::GetPickedActor() const
    {
        if (m_pPicker != nullptr)
        {
            CInstanceBase* pPicked = m_pPicker->GetPickedActor();
            if (pPicked != nullptr)
                return pPicked;
        }

        // Delegacja awaryjna
        return CPythonCharacterManager::Instance().GetPickedActorPtr();
    }

    bool ActorProviderAdapter::IsActorAlive(uint32_t dwVID) const
    {
        if (dwVID == 0)
            return false;

        CInstanceBase* pActor = GetInstance(dwVID);
        if (pActor == nullptr)
            return false;

        return (pActor->IsDead() == FALSE);
    }

    bool ActorProviderAdapter::GetActorPosition(uint32_t dwVID, TPixelPosition* pOutPos) const
    {
        if (dwVID == 0 || pOutPos == nullptr)
            return false;

        CInstanceBase* pActor = GetInstance(dwVID);
        if (pActor == nullptr)
            return false;

        pActor->NEW_GetPixelPosition(pOutPos);
        return true;
    }

    float ActorProviderAdapter::GetDistance(uint32_t dwVID1, uint32_t dwVID2) const
    {
        if (dwVID1 == 0 || dwVID2 == 0)
            return -1.0f;

        if (dwVID1 == dwVID2)
        {
            return (GetInstance(dwVID1) != nullptr) ? 0.0f : -1.0f;
        }

        CInstanceBase* pActor1 = GetInstance(dwVID1);
        CInstanceBase* pActor2 = GetInstance(dwVID2);
        if (pActor1 == nullptr || pActor2 == nullptr)
            return -1.0f;

        TPixelPosition pos1(0.0f, 0.0f, 0.0f);
        TPixelPosition pos2(0.0f, 0.0f, 0.0f);

        pActor1->NEW_GetPixelPosition(&pos1);
        pActor2->NEW_GetPixelPosition(&pos2);

        float dx = pos1.x - pos2.x;
        float dy = pos1.y - pos2.y;
        float dz = pos1.z - pos2.z;

        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    bool ActorProviderAdapter::IsSamePartyMember(uint32_t dwVID1, uint32_t dwVID2) const
    {
        if (dwVID1 == 0 || dwVID2 == 0)
            return false;

        if (IAbstractPlayer::GetSingletonPtr() != nullptr)
        {
            return IAbstractPlayer::GetSingleton().IsSamePartyMember(dwVID1, dwVID2);
        }

        // Alternatywna weryfikacja na podstawie flag instancji
        CInstanceBase* pActor1 = GetInstance(dwVID1);
        CInstanceBase* pActor2 = GetInstance(dwVID2);
        if (pActor1 != nullptr && pActor2 != nullptr)
        {
            return (pActor1->IsPartyMember() && pActor2->IsPartyMember());
        }

        return false;
    }

    void ActorProviderAdapter::SetRegistry(const ActorRegistry* pRegistry) noexcept
    {
        m_pRegistry = pRegistry;
    }

    const ActorRegistry* ActorProviderAdapter::GetRegistry() const noexcept
    {
        return m_pRegistry;
    }

    void ActorProviderAdapter::SetPicker(const CharacterPicker* pPicker) noexcept
    {
        m_pPicker = pPicker;
    }

    const CharacterPicker* ActorProviderAdapter::GetPicker() const noexcept
    {
        return m_pPicker;
    }

    void ActorProviderAdapter::SetMainActorVID(uint32_t dwVID) noexcept
    {
        m_mainActorVID = dwVID;
    }

    uint32_t ActorProviderAdapter::GetMainActorVID() const noexcept
    {
        return m_mainActorVID;
    }
}
