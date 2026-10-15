/**
 * @file ExchangeStartHandler.h
 * @brief Nowoczesny handler C++23 dla pakietu rozpoczecia wymiany.
 */

#pragma once

#include <cstdint>
#include <span>
#include <string>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

namespace Network::Handlers {

/**
 * @brief Zdarzenie publikowane na szynie po pomyslnym rozpoczeciu sesji handlu.
 */
struct ExchangeStartEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId targetId;
    std::string targetName;

    /**
     * @brief Konstruktor zdarzenia ExchangeStartEvent.
     * @param id Identyfikator (VID) celu wymiany.
     * @param name Nazwa celu wymiany.
     */
    ExchangeStartEvent(EterBase::EntityId id, std::string name)
        : targetId(id), targetName(std::move(name)) {}
};

/**
 * @brief Przetwarza pakiet sieciowy inicjujacy sesje handlu (wymiane) miedzy graczami.
 * 
 * Funkcja weryfikuje dlugosc bufora, bezpiecznie aktualizuje stan `CPythonExchange`
 * w pamieci C++ oraz publikuje zdarzenie poprzez `EventBus`.
 * 
 * @param buffer Zrzut pamieci pakietu sieciowego (span na bajty).
 * @return EterBase::PacketResult<void> Sukces operacji lub odpowiedni kod bledu.
 */
EterBase::PacketResult<void> HandleExchangeStart(std::span<const uint8_t> buffer);

} // namespace Network::Handlers
