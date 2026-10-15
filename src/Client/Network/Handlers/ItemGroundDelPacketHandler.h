#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "Client/Core/EventBus.h"

namespace Client::Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu usuniecia przedmiotu z ziemi (TPacketGCItemGroundDel).
     */
    struct PacketItemGroundDel
    {
        uint8_t  header;
        uint32_t dwVID;
    };
#pragma pack(pop)

    static_assert(sizeof(PacketItemGroundDel) == 5, "PacketItemGroundDel must be exactly 5 bytes");

    /**
     * @brief Zdarzenie domenowe emitowane po otrzymaniu poprawnego pakietu ITEM_GROUND_DEL.
     * Zgodnie z architektura Zero-Conflict, systemy sa odseparowane przez EventBus.
     */
    struct ItemGroundDelEvent : public Client::Core::IEvent {
        EterBase::EntityId dropVid;

        explicit ItemGroundDelEvent(EterBase::EntityId vid) : dropVid(vid) {}
    };

    /**
     * @brief Dekoduje pakiet i dystrybuuje go w domenie za pomoca EventBus (Dependency Injection).
     * @param buffer Bufor bajtow pakietu.
     * @return EterBase::PacketResult<void> ze statusem.
     */
    EterBase::PacketResult<void> ProcessItemGroundDel(std::span<const uint8_t> buffer);
}
