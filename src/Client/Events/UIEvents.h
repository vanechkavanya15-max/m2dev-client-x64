#pragma once

#include "EterBase/EventBus.h"
#include <expected>
#include <cstdint>
#include <string_view>

namespace Client::Events {

/**
 * @brief Reprezentuje blad walidacji zdarzen interfejsu uzytkownika.
 */
enum class UIEventError : uint8_t {
    InvalidResolution,
    InvalidWindowId
};

/**
 * @brief Zdarzenie wywolywane, gdy zmianie ulega rozdzielczosc okna gry (lub przejscie w tryb pelnoekranowy).
 */
struct ScreenResolutionChangedEvent final : public ::EterBase::IEvent {
    uint32_t width;
    uint32_t height;
    bool isFullscreen;

    constexpr ScreenResolutionChangedEvent(uint32_t w, uint32_t h, bool full) noexcept
        : width(w), height(h), isFullscreen(full) {}

    constexpr std::expected<void, UIEventError> Validate() const noexcept {
        if (width == 0 || height == 0) {
            return std::unexpected(UIEventError::InvalidResolution);
        }
        return {};
    }
};

/**
 * @brief Zdarzenie wywolywane przy zmianie fokusu wewnetrznego okienka UI (np. okienko ekwipunku, statusu).
 */
struct UIWindowFocusChangedEvent final : public ::EterBase::IEvent {
    uint32_t windowId;
    bool hasFocus;

    constexpr UIWindowFocusChangedEvent(uint32_t id, bool focus) noexcept
        : windowId(id), hasFocus(focus) {}
        
    constexpr std::expected<void, UIEventError> Validate() const noexcept {
        // Identyfikator 0 traktujemy jako nieprawidlowy
        if (windowId == 0) {
            return std::unexpected(UIEventError::InvalidWindowId);
        }
        return {};
    }
};

} // namespace Client::Events
