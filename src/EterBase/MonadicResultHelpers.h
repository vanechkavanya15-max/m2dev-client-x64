#pragma once

#include <expected>
#include <optional>
#include <functional>
#include <type_traits>
#include <string_view>
#include <format>
#include "Result.h"
#include "LogModern.h"
#include "StrongTypes.h"

/**
 * @file MonadicResultHelpers.h
 * @brief Rozszerzenia potokow monadycznych dla std::expected oraz std::optional.
 * 
 * Modul dostarcza funkcje pomocnicze do manipulacji typami std::expected, 
 * umozliwiajac m.in. logowanie bledow, mapowanie do/z std::optional oraz
 * bezinwazyjne wykonywanie efektow ubocznych (np. publikowanie zdarzen).
 */

namespace EterBase {

/**
 * @brief Wykonuje akcje (efekt uboczny) w przypadku sukcesu i zwraca oryginalny rezultat.
 * 
 * @tparam T Typ wartosci oczekiwanej.
 * @tparam E Typ bledu.
 * @tparam F Typ funkcji/funktora.
 * @param result Rezultat (std::expected) do przetworzenia.
 * @param func Funkcja do wywolania, jesli result zawiera wartosc.
 * @return std::expected<T, E> Zwraca nienaruszony, wejsciowy result.
 */
template <typename T, typename E, typename F>
constexpr std::expected<T, E> TapSuccess(std::expected<T, E> result, F&& func) {
    if (result.has_value()) {
        if constexpr (std::is_void_v<T>) {
            std::invoke(std::forward<F>(func));
        } else {
            std::invoke(std::forward<F>(func), result.value());
        }
    }
    return result;
}

/**
 * @brief Wykonuje akcje (efekt uboczny) w przypadku bledu i zwraca oryginalny rezultat.
 * 
 * @tparam T Typ wartosci oczekiwanej.
 * @tparam E Typ bledu.
 * @tparam F Typ funkcji/funktora (wymagany argument typu E).
 * @param result Rezultat (std::expected) do przetworzenia.
 * @param func Funkcja do wywolania, jesli result zawiera blad.
 * @return std::expected<T, E> Zwraca nienaruszony, wejsciowy result.
 */
template <typename T, typename E, typename F>
constexpr std::expected<T, E> TapError(std::expected<T, E> result, F&& func) {
    if (!result.has_value()) {
        std::invoke(std::forward<F>(func), result.error());
    }
    return result;
}

/**
 * @brief Loguje blad, jesli rezultat jest niepowodzeniem, uzywajac formatowania.
 * 
 * @tparam T Typ wartosci oczekiwanej.
 * @tparam E Typ bledu.
 * @tparam Args Typy argumentow formatowania.
 * @param result Rezultat (std::expected) do sprawdzenia.
 * @param level Poziom logowania (domyslnie Error).
 * @param fmt Ciag formatujacy.
 * @param args Dodatkowe argumenty dla std::format.
 * @return std::expected<T, E> Zwraca nienaruszony, wejsciowy result.
 */
template <typename T, typename E, typename... Args>
constexpr std::expected<T, E> LogOnErrorWithFormat(std::expected<T, E> result, LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
    if (!result.has_value()) {
        ModernLogger::Log(level, fmt, std::forward<Args>(args)...);
    }
    return result;
}

/**
 * @brief Loguje blad (automatyczne formatowanie bledu) w przypadku niepowodzenia.
 * 
 * @tparam T Typ wartosci oczekiwanej.
 * @tparam E Typ bledu, musi byc formatowalny.
 * @param result Rezultat do sprawdzenia.
 * @param level Poziom logowania (domyslnie Error).
 * @param prefix Opcjonalny prefiks dla bledu.
 * @return std::expected<T, E> Zwraca nienaruszony result.
 */
template <typename T, typename E>
constexpr std::expected<T, E> LogOnError(std::expected<T, E> result, LogLevel level = LogLevel::Error, std::string_view prefix = "Error: ") {
    if (!result.has_value()) {
        ModernLogger::Log(level, "{}{}", prefix, result.error());
    }
    return result;
}

/**
 * @brief Konwertuje std::optional na std::expected.
 * 
 * @tparam T Typ wartosci w opcjonalu.
 * @tparam E Typ bledu.
 * @param opt Opcjonal do konwersji.
 * @param error_value Wartosc bledu, jesli opt jest pusty.
 * @return std::expected<T, E> Rezultat z wartoscia lub z bledem.
 */
template <typename T, typename E>
constexpr std::expected<T, E> FromOptional(std::optional<T> opt, E error_value) {
    if (opt.has_value()) {
        return opt.value();
    }
    return std::unexpected(std::move(error_value));
}

/**
 * @brief Konwertuje std::expected na std::optional.
 * 
 * @tparam T Typ wartosci oczekiwanej (nie moze byc void).
 * @tparam E Typ bledu.
 * @param result Rezultat do konwersji.
 * @return std::optional<T> Opcjonal z wartoscia, jesli sukces.
 */
template <typename T, typename E>
    requires (!std::is_void_v<T>)
constexpr std::optional<T> ToOptional(const std::expected<T, E>& result) {
    if (result.has_value()) {
        return result.value();
    }
    return std::nullopt;
}

/**
 * @brief Publikuje zdarzenie w EventBus w przypadku sukcesu.
 * 
 * Wspiera architekture Event-Driven bez bezposredniej zaleznosci od interfejsu UI.
 * 
 * @tparam T Typ wartosci oczekiwanej.
 * @tparam E Typ bledu.
 * @tparam Bus Typ szyny zdarzen (np. EventBus).
 * @tparam Event Typ zdarzenia.
 * @param result Rezultat.
 * @param bus Referencja na szyne zdarzen.
 * @param event Instancja zdarzenia do opublikowania.
 * @return std::expected<T, E> Nienaruszony result.
 */
template <typename T, typename E, typename Bus, typename Event>
constexpr std::expected<T, E> PublishOnSuccess(std::expected<T, E> result, Bus& bus, Event&& event) {
    if (result.has_value()) {
        bus.Publish(std::forward<Event>(event));
    }
    return result;
}

// ============================================================================
// Walidatory Monadyczne dla Silnych Typow (Strong Types)
// ============================================================================

/**
 * @brief Sprawdza poprawnosc identyfikatora i zwraca monadyczny blad w razie pustej wartosci.
 * 
 * @param id Identyfikator bytu (EntityId).
 * @return Result<EntityId, EntityError> EntityId w przypadku poprawnosci.
 */
constexpr Result<EntityId, EntityError> ValidateEntity(EntityId id) noexcept {
    if (static_cast<bool>(id)) {
        return id;
    }
    return MakeError(EntityError::NotFound);
}

/**
 * @brief Sprawdza poprawnosc VNUM przedmiotu.
 * 
 * @param vnum Wirtualny numer przedmiotu.
 * @return Result<ItemVnum, InventoryError> W przypadku poprawnosci zwraca ItemVnum.
 */
constexpr Result<ItemVnum, InventoryError> ValidateItem(ItemVnum vnum) noexcept {
    if (static_cast<bool>(vnum)) {
        return vnum;
    }
    return MakeError(InventoryError::InvalidVnum);
}

/**
 * @brief Sprawdza poprawnosc slotu ekwipunku.
 * 
 * @param slot Identyfikator slotu.
 * @return Result<ItemSlot, InventoryError> W przypadku poprawnosci zwraca ItemSlot.
 */
constexpr Result<ItemSlot, InventoryError> ValidateSlot(ItemSlot slot) noexcept {
    if (static_cast<bool>(slot)) {
        return slot;
    }
    return MakeError(InventoryError::SlotOutOfRange);
}

/**
 * @brief Sprawdza poprawnosc umiejetnosci.
 * 
 * @param skillId Identyfikator umiejetnosci.
 * @return Result<SkillId, CombatError> W przypadku poprawnosci zwraca SkillId.
 */
constexpr Result<SkillId, CombatError> ValidateSkill(SkillId skillId) noexcept {
    if (static_cast<bool>(skillId)) {
        return skillId;
    }
    return MakeError(CombatError::InvalidAction);
}

} // namespace EterBase
