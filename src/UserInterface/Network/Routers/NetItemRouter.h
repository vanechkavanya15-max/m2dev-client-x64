#pragma once

#include "../../Contracts/IPacketRouter.h"
#include "../../Contracts/IGameEvents.h"
#include "Client/Network/Protocol/Protocol.h"
#include "../../Core/EventBus.h"
#include <string_view>
#include <cstdint>

namespace UserInterface::Network::Routers
{
    // Struktury zdarzen domenowych dla szyny EventBus
    struct ItemSetDomainEvent : public UserInterface::Core::IEvent
    {
        TItemPos Cell;
        uint32_t dwVnum;
        uint8_t byCount;
        uint32_t dwFlags;
        uint32_t dwAntiFlags;
        uint8_t byHighlight;

        ItemSetDomainEvent(const TItemPos& pos, uint32_t vnum, uint8_t count, uint32_t flags, uint32_t antiFlags, uint8_t highlight)
            : Cell(pos), dwVnum(vnum), byCount(count), dwFlags(flags), dwAntiFlags(antiFlags), byHighlight(highlight) {}
    };

    struct ItemDelDomainEvent : public UserInterface::Core::IEvent
    {
        TItemPos Cell;

        explicit ItemDelDomainEvent(const TItemPos& pos) : Cell(pos) {}
    };

    struct ItemGroundAddDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwVID;
        uint32_t dwVnum;
        int32_t lX;
        int32_t lY;
        int32_t lZ;

        ItemGroundAddDomainEvent(uint32_t vid, uint32_t vnum, int32_t x, int32_t y, int32_t z)
            : dwVID(vid), dwVnum(vnum), lX(x), lY(y), lZ(z) {}
    };

    struct ItemGroundDelDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwVID;

        explicit ItemGroundDelDomainEvent(uint32_t vid) : dwVID(vid) {}
    };

    struct QuickSlotAddDomainEvent : public UserInterface::Core::IEvent
    {
        uint8_t byPos;
        TQuickSlot Slot;

        QuickSlotAddDomainEvent(uint8_t pos, const TQuickSlot& slot)
            : byPos(pos), Slot(slot) {}
    };

    struct QuickSlotDelDomainEvent : public UserInterface::Core::IEvent
    {
        uint8_t byPos;

        explicit QuickSlotDelDomainEvent(uint8_t pos) : byPos(pos) {}
    };

    struct QuickSlotSwapDomainEvent : public UserInterface::Core::IEvent
    {
        uint8_t byPos;
        uint8_t byChangePos;

        QuickSlotSwapDomainEvent(uint8_t pos, uint8_t changePos)
            : byPos(pos), byChangePos(changePos) {}
    };

    /**
     * @brief Router domenowy odpowiedzialny za obsluge zdeserializowanych pakietow przedmiotow.
     * 
     * KONTRAKT ZERO-DESYNC:
     * Router NIGDY nie dotyka surowego bufora TCP i NIGDY nie wywoluje Recv().
     * Przyjmuje wylacznie zdeserializowane rekordy POD przez const referencje.
     * Dziala wylacznie w glownym watku (single-threaded).
     */
    class NetItemRouter : public UserInterface::Contracts::IPacketRouter
    {
    public:
        NetItemRouter() noexcept = default;
        explicit NetItemRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept;
        ~NetItemRouter() override = default;

        // Implementacja kontraktu IPacketRouter
        [[nodiscard]] std::string_view GetRouterName() const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint8_t bHeader) const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint16_t wHeader) const noexcept;

        // Rejestracja odbiornika zdarzen domenowych
        void SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept;
        [[nodiscard]] UserInterface::Contracts::IGameEventSink* GetEventSink() const noexcept;

        // Metody dyspozycji zdeserializowanych pakietow przedmiotow
        void HandleItemSet(const TPacketGCItemSet& packet);
        void HandleItemDel(const TPacketGCItemDel& packet);
        void HandleItemGroundAdd(const TPacketGCItemGroundAdd& packet);
        void HandleItemGroundDel(const TPacketGCItemGroundDel& packet);
        void HandleQuickSlotAdd(const TPacketGCQuickSlotAdd& packet);
        void HandleQuickSlotDel(const TPacketGCQuickSlotDel& packet);
        void HandleQuickSlotSwap(const TPacketGCQuickSlotSwap& packet);

    private:
        UserInterface::Contracts::IGameEventSink* m_pEventSink{nullptr};
    };
}

// Globalny alias w przestrzeni Network::Routers dla wygody
namespace Network::Routers
{
    using NetItemRouter = UserInterface::Network::Routers::NetItemRouter;
}
