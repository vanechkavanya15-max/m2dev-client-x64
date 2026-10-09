#pragma once

#include "../Core/EngineForwardDecls.h"
#include <cstdint>

namespace UserInterface::Contracts
{
    class IActorProvider
    {
    public:
        virtual ~IActorProvider() = default;

        // Pobranie instancji na biezaca klatke (ZAKAZ CACHOWANIA WSKAZNIKA!)
        virtual CInstanceBase* GetInstance(uint32_t dwVID) const = 0;
        virtual CInstanceBase* GetMainActor() const = 0;
        virtual CInstanceBase* GetPickedActor() const = 0;

        // Zapytania bezpieczne (bez wyciagania wskaznika)
        virtual bool IsActorAlive(uint32_t dwVID) const = 0;
        virtual bool GetActorPosition(uint32_t dwVID, TPixelPosition* pOutPos) const = 0;
        virtual float GetDistance(uint32_t dwVID1, uint32_t dwVID2) const = 0;
        virtual bool IsSamePartyMember(uint32_t dwVID1, uint32_t dwVID2) const = 0;
    };
}
