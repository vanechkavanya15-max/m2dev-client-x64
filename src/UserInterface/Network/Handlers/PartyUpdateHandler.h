#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

namespace Network::Handlers
{
    /**
     * @brief Zdarzenie aktualizacji czlonka grupy dla EventBus.
     * Emitowane po odebraniu i przetworzeniu pakietu aktualizacji grupy.
     */
    struct PartyUpdateEvent : public UserInterface::Core::IEvent
    {
        uint32_t pid;

        /**
         * @brief Konstruktor zdarzenia.
         * @param pid Identyfikator PID czlonka grupy.
         */
        explicit PartyUpdateEvent(uint32_t pid) : pid(pid) {}
    };

#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu aktualizacji danych o czlonku grupy (TPacketGCPartyUpdate).
     */
    struct PacketPartyUpdate
    {
        uint16_t header;
        uint16_t length;
        uint32_t pid;
        uint8_t state;
        uint8_t percent_hp;
        int16_t affects[7]; // PARTY_AFFECT_SLOT_MAX_NUM = 7
    };
#pragma pack(pop)

    /**
     * @brief Przetwarza pakiet aktualizacji czlonka grupy zwracajac PacketResult C++23.
     * @param buffer Bufor bajtow ze strumienia sieciowego.
     * @return EterBase::PacketResult<void> ze statusem powodzenia lub bledu.
     */
    EterBase::PacketResult<void> ProcessPartyUpdate(std::span<const uint8_t> buffer);

    /**
     * @brief Przetwarza pakiet aktualizacji czlonka grupy (interfejs kompatybilny).
     * @param buffer Bufor bajtow ze strumienia sieciowego.
     * @return true w przypadku poprawnego przetworzenia.
     */
    bool HandlePartyUpdate(std::span<const uint8_t> buffer);
}
