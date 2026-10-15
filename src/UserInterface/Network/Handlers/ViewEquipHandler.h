#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"
#include "../../Packet.h"

namespace Network::Handlers
{
    /**
     * @brief Zdarzenie emitowane po otrzymaniu pakietu podglądu ekwipunku (HEADER_GC_VIEW_EQUIP).
     */
    struct ViewEquipEvent : public UserInterface::Core::IEvent {
        EterBase::EntityId targetVid;
        std::span<const TEquipmentItemSet> equips;

        /**
         * @brief Konstruktor zdarzenia ViewEquipEvent.
         * @param targetVid Identyfikator celu (VID), którego ekwipunek jest przeglądany.
         * @param equips Zakres wyekwipowanych przedmiotów.
         */
        explicit ViewEquipEvent(EterBase::EntityId targetVid, std::span<const TEquipmentItemSet> equips)
            : targetVid(targetVid), equips(equips) {}
    };

    /**
     * @brief Przetwarza pakiet podglądu ekwipunku gracza zwracając PacketResult C++23.
     * @param buffer Bufor bajtów pakietu.
     * @return EterBase::PacketResult<void> ze statusem sukcesu lub błędu.
     */
    EterBase::PacketResult<void> ProcessViewEquip(std::span<const uint8_t> buffer);
}
