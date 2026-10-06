#pragma once

#include <cstdint>
#include <span>

#include "../../Core/EventBus.h"
#include "../../../EterBase/Result.h"

namespace Network::Handlers
{

/**
 * @struct ExchangeItemDelEvent
 * @brief Zdarzenie usuwania przedmiotu z okna wymiany dla UI (wzorzec Event-Driven).
 * 
 * Emitowane przez HandleExchangeItemDel po poprawnej weryfikacji i
 * aktualizacji stanu w CPythonExchange. Zastepuje archaiczne, bezposrednie 
 * wywolania Pythona.
 */
struct ExchangeItemDelEvent : public UserInterface::Core::IEvent
{
    uint8_t slotIndex;
    bool isMe;

    /**
     * @brief Konstruktor zdarzenia
     * @param slotIndex Numer slotu, z ktorego usunieto przedmiot.
     * @param isMe True, jesli to przedmiot gracza inicjujacego, false dla celu.
     */
    ExchangeItemDelEvent(uint8_t slotIndex, bool isMe)
        : slotIndex(slotIndex), isMe(isMe) {}
};

/**
 * @brief Przetwarza pakiet usuniecia przedmiotu z okna wymiany (C++20).
 * 
 * @param buffer Bufor bajtow zawierajacy strukture TPacketGCExchange.
 * @return Zwraca sukces lub EterBase::PacketError w przypadku bledu.
 */
EterBase::PacketResult<void> HandleExchangeItemDel(std::span<const uint8_t> buffer);

} // namespace Network::Handlers
