#pragma once

#include <cstdint>
#include <span>
#include <string>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../World/ECSComponents.h"
#include "../../../UserInterface/Core/EventBus.h"

namespace Client::Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu dodania przedmiotu na ziemi (TPacketGCItemGroundAdd).
     */
    struct PacketItemGroundAdd
    {
        uint16_t header;
        uint16_t length;
        int32_t  lX;
        int32_t  lY;
        int32_t  lZ;
        uint32_t dwVID;
        uint32_t dwVnum;
    };
#pragma pack(pop)

    /**
     * @brief Zdarzenie domenowe emitowane po otrzymaniu poprawnego pakietu ITEM_GROUND_ADD.
     * Zgodnie z architekturą Zero-Conflict, systemy są odseparowane przez EventBus.
     */
    struct ItemGroundAddEvent : public UserInterface::Core::IEvent {
        EterBase::EntityId dropVid;
        EterBase::ItemVnum itemVnum;
        Client::World::MapCoords coords;
        // Opcjonalna etykieta wlasnosci dodana dla zgodnosci z celem
        std::string ownershipLabel;

        ItemGroundAddEvent(EterBase::EntityId vid, EterBase::ItemVnum vnum, Client::World::MapCoords c, std::string ownership = "")
            : dropVid(vid), itemVnum(vnum), coords(c), ownershipLabel(std::move(ownership)) {}
    };

    /**
     * @brief Dekoduje pakiet i dystrybuuje go w domenie za pomoca EventBus (Dependency Injection).
     * @param buffer Bufor bajtow pakietu.
     * @return EterBase::PacketResult<void> ze statusem.
     */
    EterBase::PacketResult<void> ProcessItemGroundAdd(std::span<const uint8_t> buffer);
}
