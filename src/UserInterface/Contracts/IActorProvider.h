#pragma once

#include "../Core/EngineForwardDecls.h"
#include "Client/World/ActorRegistry.h"
#include <cstdint>

namespace UserInterface::Contracts
{
    class IActorProvider
    {
    public:
        using EntityVid = Client::World::EntityVid;

        virtual ~IActorProvider() = default;

        // Pobranie instancji na biezaca klatke (ZAKAZ CACHOWANIA WSKAZNIKA!)
        [[nodiscard]] virtual CInstanceBase* GetInstance(uint32_t vid) const = 0;
        [[nodiscard]] virtual CInstanceBase* GetInstance(EntityVid vid) const { return GetInstance(vid.get()); }
        [[nodiscard]] virtual CInstanceBase* GetMainActor() const = 0;
        [[nodiscard]] virtual CInstanceBase* GetPickedActor() const = 0;

        // Zapytania bezpieczne (bez wyciagania wskaznika)
        [[nodiscard]] virtual bool IsActorAlive(uint32_t vid) const = 0;
        [[nodiscard]] virtual bool IsActorAlive(EntityVid vid) const { return IsActorAlive(vid.get()); }
        [[nodiscard]] virtual bool GetActorPosition(uint32_t vid, TPixelPosition* outPos) const = 0;
        [[nodiscard]] virtual bool GetActorPosition(EntityVid vid, TPixelPosition* outPos) const { return GetActorPosition(vid.get(), outPos); }
        [[nodiscard]] virtual float GetDistance(uint32_t vid1, uint32_t vid2) const = 0;
        [[nodiscard]] virtual float GetDistance(EntityVid vid1, EntityVid vid2) const { return GetDistance(vid1.get(), vid2.get()); }
        [[nodiscard]] virtual bool IsSamePartyMember(uint32_t vid1, uint32_t vid2) const = 0;
        [[nodiscard]] virtual bool IsSamePartyMember(EntityVid vid1, EntityVid vid2) const { return IsSamePartyMember(vid1.get(), vid2.get()); }
    };
}
