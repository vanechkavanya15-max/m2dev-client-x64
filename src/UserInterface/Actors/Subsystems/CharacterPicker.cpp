#include "StdAfx.h"
#include "CharacterPicker.h"
#include "ActorRegistry.h"
#include "InstanceBase.h"
#include "EterLib/Camera.h"
#include "EterPythonLib/PythonGraphic.h"
#include "EterBase/LogModern.h"

#include <algorithm>

namespace UserInterface::Actors::Subsystems
{
    CharacterPicker::CharacterPicker(const ActorRegistry* pRegistry) noexcept
        : m_pRegistry(pRegistry)
    {
    }

    CharacterPicker::~CharacterPicker()
    {
        ClearPick();
    }

    CInstanceBase* CharacterPicker::Pick(long mouseX, long mouseY, const CRay& cameraRay)
    {
        m_lastMouseX = mouseX;
        m_lastMouseY = mouseY;

        if (m_pRegistry == nullptr || m_pRegistry->IsEmpty())
        {
            ClearPick();
            return nullptr;
        }

        D3DXVECTOR3 eyeStart(0.0f, 0.0f, 0.0f);
        cameraRay.GetStartPoint(&eyeStart);
        TPixelPosition eyePixelPos(+eyeStart.x, -eyeStart.y, +eyeStart.z);

        std::vector<PickCandidate> candidates;
        candidates.reserve(16);

        // Przegladamy wszystkich aktorow w rejestrze
        for (const auto& [vid, pActor] : m_pRegistry->GetAllActors())
        {
            if (pActor == nullptr)
                continue;

            if (!pActor->CanPickInstance())
                continue;

            float u = 0.0f, v = 0.0f, t = 0.0f;
            bool isIntersected = false;

            // 1. Precyzyjny test promienia wzgledem bryly siatki / AABB
            if (pActor->GetGraphicThingInstancePtr() != nullptr &&
                pActor->GetGraphicThingInstancePtr()->isIntersect(cameraRay, &u, &v, &t))
            {
                isIntersected = true;
            }
            // 2. Test bounding boxu instancji
            else if (pActor->IntersectBoundingBox())
            {
                isIntersected = true;
                t = pActor->CalculateDistanceSq3d(eyePixelPos);
            }
            // 3. Dla zywych postaci test kuli obronnej (DefendingSphere)
            else if (!pActor->IsDead() && pActor->IntersectDefendingSphere())
            {
                isIntersected = true;
                t = pActor->CalculateDistanceSq3d(eyePixelPos);
            }

            if (isIntersected)
            {
                float distSq = pActor->CalculateDistanceSq3d(eyePixelPos);

                PickCandidate candidate;
                candidate.pActor = pActor;
                candidate.isDead = (pActor->IsDead() != FALSE);
                candidate.isMainActor = (pActor == m_pMainActor);
                candidate.distanceSq = distSq;
                candidate.rayIntersectionT = t;
                candidates.push_back(candidate);
            }
        }

        if (candidates.empty())
        {
            ClearPick();
            return nullptr;
        }

        // Sortowanie kandydatow wedlug priorytetow:
        // 1. Inne postacie maja pierwszenstwo przed glownym graczem (m_pMainActor)
        // 2. Zywe postacie maja pierwszenstwo przed martwymi cialami
        // 3. Mniejsza odleglosc 3D do oka kamery
        std::sort(candidates.begin(), candidates.end(), [](const PickCandidate& a, const PickCandidate& b)
        {
            if (a.isMainActor != b.isMainActor)
                return !a.isMainActor && b.isMainActor;

            if (a.isDead != b.isDead)
                return !a.isDead && b.isDead;

            return a.distanceSq < b.distanceSq;
        });

        CInstanceBase* pBestActor = candidates.front().pActor;

        // Aktualizacja stanu wybranego aktora
        if (m_pPickedActor != pBestActor)
        {
            if (m_pPickedActor != nullptr)
            {
                m_pPickedActor->OnUnselected();
            }

            m_pPickedActor = pBestActor;

            if (m_pPickedActor != nullptr)
            {
                m_pPickedActor->OnSelected();
            }
        }

        // Wyliczenie pozycji ekranowej dla celow GUI
        if (m_pPickedActor != nullptr)
        {
            const D3DXVECTOR3& worldPos = m_pPickedActor->GetGraphicThingInstanceRef().GetPosition();
            CPythonGraphic::Instance().ProjectPosition(
                worldPos.x, worldPos.y, worldPos.z,
                &m_pickedScreenPos.x, &m_pickedScreenPos.y
            );
            m_pickedDistance = candidates.front().distanceSq;
        }

        return m_pPickedActor;
    }

    CInstanceBase* CharacterPicker::Pick(long mouseX, long mouseY)
    {
        CCamera* pCamera = CCameraManager::Instance().GetCurrentCamera();
        if (pCamera != nullptr)
        {
            return Pick(mouseX, mouseY, pCamera->GetViewRay());
        }

        ClearPick();
        return nullptr;
    }

    CInstanceBase* CharacterPicker::GetPickedActor() const noexcept
    {
        return m_pPickedActor;
    }

    void CharacterPicker::ClearPick() noexcept
    {
        if (m_pPickedActor != nullptr)
        {
            m_pPickedActor->OnUnselected();
            m_pPickedActor = nullptr;
        }

        m_pickedScreenPos = D3DXVECTOR2(0.0f, 0.0f);
        m_pickedDistance = -1.0f;
    }

    void CharacterPicker::SetActorRegistry(const ActorRegistry* pRegistry) noexcept
    {
        m_pRegistry = pRegistry;
    }

    const ActorRegistry* CharacterPicker::GetActorRegistry() const noexcept
    {
        return m_pRegistry;
    }

    void CharacterPicker::SetMainActor(CInstanceBase* pMainActor) noexcept
    {
        m_pMainActor = pMainActor;
    }

    CInstanceBase* CharacterPicker::GetMainActor() const noexcept
    {
        return m_pMainActor;
    }

    const D3DXVECTOR2& CharacterPicker::GetPickedActorScreenPos() const noexcept
    {
        return m_pickedScreenPos;
    }

    float CharacterPicker::GetPickedDistance() const noexcept
    {
        return m_pickedDistance;
    }

    bool CharacterPicker::HasPickedActor() const noexcept
    {
        return m_pPickedActor != nullptr;
    }

    void CharacterPicker::OnActorRemoved(DWORD dwVID) noexcept
    {
        // DANGLING POINTER SAFETY:
        // Jezeli usuwany aktor byl wybrany jako picked, natychmiastowo go odpinamy.
        if (m_pPickedActor != nullptr && m_pPickedActor->GetVirtualID() == dwVID)
        {
            m_pPickedActor = nullptr;
            m_pickedScreenPos = D3DXVECTOR2(0.0f, 0.0f);
            m_pickedDistance = -1.0f;
        }
    }
}
