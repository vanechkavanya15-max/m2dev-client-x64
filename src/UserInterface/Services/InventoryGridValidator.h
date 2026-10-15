#pragma once

#include "IInventoryService.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../Core/EventBus.h"

namespace UserInterface::Services
{
    struct ValidationFailureEvent : public UserInterface::Core::IEvent {
        EterBase::ItemSlot slot{0};
        EterBase::ItemVnum vnum{0};
    };

    /**
     * @brief Klasa narzedziowa do walidacji wolnych komorek w siatce inwentarza dla przedmiotu.
     */
    class InventoryGridValidator
    {
    public:
        /**
         * @brief Sprawdza, czy przedmiot o podanym vnum zmiesci sie na wskazanej pozycji.
         * 
         * @param inventoryService Instancja serwisu ekwipunku dostarczajaca stan.
         * @param slot Zgloszony slot docelowy.
         * @param vnum Vnum przedmiotu, na podstawie ktorego okreslany jest jego rozmiar.
         * @return EterBase::PacketResult<void> Zwraca sukces lub blad.
         */
        static EterBase::PacketResult<void> ValidateItemPlacement(
            const IInventoryService& inventoryService,
            EterBase::ItemSlot slot,
            EterBase::ItemVnum vnum);
            
        /**
         * @brief Sprawdza, czy przedmioty o danym rozmiarze zmieszcza sie w danym slocie.
         *        Uwzglednia wysokosc (itemSize) sprawdzajac nizej (slot + 5).
         *        Zwraca false, jesli przedmiot wychodzi poza strone.
         * 
         * @param inventoryService Instancja serwisu ekwipunku dostarczajaca stan.
         * @param slot Zgloszony slot docelowy.
         * @param itemSize Rozmiar przedmiotu w pionie (1, 2, 3).
         * @return bool True jesli mozna umiescic przedmiot, false w przeciwnym razie.
         */
        static bool CanPlaceItemSize(
            const IInventoryService& inventoryService,
            EterBase::ItemSlot slot,
            uint8_t itemSize);
    };
}
