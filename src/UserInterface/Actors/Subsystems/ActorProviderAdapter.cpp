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

    CInstanceBase* ActorProviderAdapter::GetInstance(uint32_t vid) const
    {
        if (vid == 0)
            return nullptr;

        if (m_pRegistry != nullptr)
        {
            return m_pRegistry->FindActor(vid);
        }

        // Delegacja awaryjna do istniejacej fasady CPythonCharacterManager
        return CPythonCharacterManager::Instance().GetInstancePtr(vid);
    }

    CInstanceBase* ActorProviderAdapter::GetMainActor() const
    {
        if (m_mainActorVID != 0)
        {
            CInstanceBase* mainActor = GetInstance(m_mainActorVID);
            if (mainActor != nullptr)
                return mainActor;
        }

        // Delegacja awaryjna
        return CPythonCharacterManager::Instance().GetMainActorPtr();
    }

    CInstanceBase* ActorProviderAdapter::GetPickedActor() const
    {
        if (m_pPicker != nullptr)
        {
            CInstanceBase* pickedActor = m_pPicker->GetPickedActor();
            if (pickedActor != nullptr)
                return pickedActor;
        }

        // Delegacja awaryjna
        return CPythonCharacterManager::Instance().GetPickedActorPtr();
    }

    bool ActorProviderAdapter::IsActorAlive(uint32_t vid) const
    {
        if (vid == 0)
            return false;

        CInstanceBase* actor = GetInstance(vid);
        if (actor == nullptr)
            return false;

        return (actor->IsDead() == FALSE);
    }

    bool ActorProviderAdapter::GetActorPosition(uint32_t vid, TPixelPosition* outPos) const
    {
        if (vid == 0 || outPos == nullptr)
            return false;

        CInstanceBase* actor = GetInstance(vid);
        if (actor == nullptr)
            return false;

        actor->NEW_GetPixelPosition(outPos);
        return true;
    }

    float ActorProviderAdapter::GetDistance(uint32_t vid1, uint32_t vid2) const
    {
        if (vid1 == 0 || vid2 == 0)
            return -1.0f;

        if (vid1 == vid2)
        {
            return (GetInstance(vid1) != nullptr) ? 0.0f : -1.0f;
        }

        CInstanceBase* actor1 = GetInstance(vid1);
        CInstanceBase* actor2 = GetInstance(vid2);
        if (actor1 == nullptr || actor2 == nullptr)
            return -1.0f;

        TPixelPosition pos1(0.0f, 0.0f, 0.0f);
        TPixelPosition pos2(0.0f, 0.0f, 0.0f);

        actor1->NEW_GetPixelPosition(&pos1);
        actor2->NEW_GetPixelPosition(&pos2);

        float dx = pos1.x - pos2.x;
        float dy = pos1.y - pos2.y;
        float dz = pos1.z - pos2.z;

        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    bool ActorProviderAdapter::IsSamePartyMember(uint32_t vid1, uint32_t vid2) const
    {
        if (vid1 == 0 || vid2 == 0)
            return false;

        if (IAbstractPlayer::GetSingletonPtr() != nullptr)
        {
            return IAbstractPlayer::GetSingleton().IsSamePartyMember(vid1, vid2);
        }

        // Alternatywna weryfikacja na podstawie flag instancji
        CInstanceBase* actor1 = GetInstance(vid1);
        CInstanceBase* actor2 = GetInstance(vid2);
        if (actor1 != nullptr && actor2 != nullptr)
        {
            return (actor1->IsPartyMember() && actor2->IsPartyMember());
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

    void ActorProviderAdapter::SetMainActorVID(uint32_t vid) noexcept
    {
        m_mainActorVID = vid;
    }

    uint32_t ActorProviderAdapter::GetMainActorVID() const noexcept
    {
        return m_mainActorVID;
    }
}
