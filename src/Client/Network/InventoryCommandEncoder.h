#pragma once

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/PacketResult.h"
#include <vector>
#include <cstdint>

namespace Client::Network {

    /**
     * @class InventoryCommandEncoder
     * @brief Koder pakietow sieciowych dla akcji na ekwipunku (Use, Drop, Pickup).
     * @details Implementuje mapowanie miedzy ItemSlot (Silny Typ C++23) a TItemPos (struktura sieciowa).
     */
    class InventoryCommandEncoder {
    public:
        /**
         * @brief Koduje zadanie uzycia przedmiotu (TPacketCGItemUse).
         * @param slot Pozycja (slot) w ekwipunku postaci.
         * @return Binarny wektor reprezentujacy strukture pakietu lub blad kodowania.
         */
        [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeUseItem(EterBase::ItemSlot slot);

        /**
         * @brief Koduje zadanie wyrzucenia przedmiotu z okreslonym limitem zlota i ilosci (TPacketCGItemDrop2).
         * @param slot Pozycja (slot) w ekwipunku postaci.
         * @param gold Opcjonalna wartosc zlota (elk).
         * @param count Ilosc wyrzucanych przedmiotow.
         * @return Binarny wektor reprezentujacy strukture pakietu lub blad kodowania.
         */
        [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeDropItem(EterBase::ItemSlot slot, uint32_t gold, uint8_t count);

        /**
         * @brief Koduje zadanie podniesienia przedmiotu z ziemi (TPacketCGItemPickUp).
         * @param itemVid Unikalny identyfikator (VID) przedmiotu na ziemi.
         * @return Binarny wektor reprezentujacy strukture pakietu lub blad kodowania.
         */
        [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodePickupItem(EterBase::EntityId itemVid);
    };

} // namespace Client::Network
