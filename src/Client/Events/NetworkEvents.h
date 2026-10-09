#pragma once

#include <cstdint>
#include <string_view>
#include <optional>
#ifndef EVENTBUS_MOCKED
#include "Client/Core/EventBus.h"
#endif

namespace Client::Events {

/**
 * @brief Enum opisujacy przyczyne rozlaczenia z serwerem.
 */
enum class DisconnectReason : uint8_t {
    Unknown = 0,
    Timeout,
    ConnectionClosed,
    Kicked,
    GracefulShutdown
};

/**
 * @brief Enum opisujacy glowne fazy gry.
 */
enum class GamePhase : uint8_t {
    Login = 0,
    Select,
    Loading,
    Game
};

/**
 * @brief Zdarzenie emitowane w przypadku zerwania polaczenia sieciowego.
 */
struct SocketDisconnectedEvent : public Client::Core::IEvent {
    DisconnectReason reason;
    std::optional<int32_t> socketErrorCode;

    constexpr SocketDisconnectedEvent(
        DisconnectReason reason = DisconnectReason::Unknown,
        std::optional<int32_t> errorCode = std::nullopt) noexcept
        : reason(reason), socketErrorCode(errorCode) {}
};

/**
 * @brief Zdarzenie emitowane po skutecznym odnowieniu polaczenia (reconnect).
 */
struct SocketReconnectedEvent : public Client::Core::IEvent {
    uint32_t attemptCount;

    constexpr explicit SocketReconnectedEvent(uint32_t attempts = 1) noexcept
        : attemptCount(attempts) {}
};

/**
 * @brief Zdarzenie emitowane przy zmianie glownej fazy aplikacji klienta.
 */
struct PhaseChangedEvent : public Client::Core::IEvent {
    GamePhase oldPhase;
    GamePhase newPhase;

    constexpr PhaseChangedEvent(GamePhase oldP, GamePhase newP) noexcept
        : oldPhase(oldP), newPhase(newP) {}
};

} // namespace Client::Events
