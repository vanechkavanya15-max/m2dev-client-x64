#pragma once

#include "../../Contracts/IPacketRouter.h"
#include "../../Contracts/IGameEvents.h"
#include "Client/Network/Protocol/Protocol.h"
#include "../../Core/EventBus.h"
#include <string_view>
#include <string>
#include <cstdint>

namespace UserInterface::Network::Routers
{
    // Struktury zdarzen domenowych dla szyny EventBus
    struct ActorAddDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwVID;
        float fAngle;
        int32_t lX;
        int32_t lY;
        int32_t lZ;
        uint8_t byType;
        uint16_t wRaceNum;
        uint8_t byMovingSpeed;
        uint8_t byAttackSpeed;
        uint8_t byStateFlag;

        ActorAddDomainEvent(uint32_t vid, float angle, int32_t x, int32_t y, int32_t z,
            uint8_t type, uint16_t race, uint8_t moveSpeed, uint8_t attackSpeed, uint8_t state)
            : dwVID(vid), fAngle(angle), lX(x), lY(y), lZ(z),
              byType(type), wRaceNum(race), byMovingSpeed(moveSpeed), byAttackSpeed(attackSpeed), byStateFlag(state) {}
    };

    struct ActorAdditionalInfoDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwVID;
        std::string strName;
        uint8_t byEmpire;
        uint32_t dwGuildID;
        uint32_t dwLevel;
        int16_t sAlignment;
        uint8_t byPKMode;
        uint32_t dwMountVnum;

        ActorAdditionalInfoDomainEvent(uint32_t vid, std::string_view name, uint8_t empire,
            uint32_t guild, uint32_t level, int16_t alignment, uint8_t pkMode, uint32_t mount)
            : dwVID(vid), strName(name), byEmpire(empire),
              dwGuildID(guild), dwLevel(level), sAlignment(alignment), byPKMode(pkMode), dwMountVnum(mount) {}
    };

    struct ActorDeleteDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwVID;

        explicit ActorDeleteDomainEvent(uint32_t vid) : dwVID(vid) {}
    };

    struct ObserverMoveDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwVID;
        uint16_t wX;
        uint16_t wY;

        ObserverMoveDomainEvent(uint32_t vid, uint16_t x, uint16_t y)
            : dwVID(vid), wX(x), wY(y) {}
    };

    struct ActorSyncPositionDomainEvent : public UserInterface::Core::IEvent
    {
        uint16_t wLength;

        explicit ActorSyncPositionDomainEvent(uint16_t length) : wLength(length) {}
    };

    /**
     * @brief Router domenowy odpowiedzialny za obsluge zdeserializowanych pakietow aktorow.
     * 
     * KONTRAKT ZERO-DESYNC:
     * Router NIGDY nie dotyka surowego bufora TCP i NIGDY nie wywoluje Recv().
     * Przyjmuje wylacznie zdeserializowane rekordy POD przez const referencje.
     * Dziala wylacznie w glownym watku (single-threaded).
     */
    class NetActorRouter : public UserInterface::Contracts::IPacketRouter
    {
    public:
        NetActorRouter() noexcept = default;
        explicit NetActorRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept;
        ~NetActorRouter() override = default;

        // Implementacja kontraktu IPacketRouter
        [[nodiscard]] std::string_view GetRouterName() const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint8_t bHeader) const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint16_t wHeader) const noexcept;

        // Rejestracja odbiornika zdarzen domenowych
        void SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept;
        [[nodiscard]] UserInterface::Contracts::IGameEventSink* GetEventSink() const noexcept;

        // Metody dyspozycji zdeserializowanych pakietow aktorow
        void HandleCharacterAdd(const TPacketGCCharacterAdd& packet);
        void HandleCharacterAdditionalInfo(const TPacketGCCharacterAdditionalInfo& packet);
        void HandleCharacterDelete(const TPacketGCCharacterDelete& packet);
        void HandleObserverMove(const TPacketGCObserverMove& packet);
        void HandleSyncPosition(const TPacketGCSyncPosition& packet);

    private:
        UserInterface::Contracts::IGameEventSink* m_pEventSink{nullptr};
    };
}

// Globalny alias w przestrzeni Network::Routers dla wygody
namespace Network::Routers
{
    using NetActorRouter = UserInterface::Network::Routers::NetActorRouter;
}
