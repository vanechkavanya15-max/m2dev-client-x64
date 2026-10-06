#pragma once

#include <expected>
#include <string_view>
#include <format>
#include <cstdint>

/**
 * @file Result.h
 * @brief Nowoczesny modul obslugi rezultatow C++23 oparty na std::expected.
 * 
 * Zastepuje archaiczne wzorce 'bool Func(OutParam* out)' oraz kody bledow 'int'.
 * Daje modelom AI i programistom 100% determinizmu w obsludze sukcesow i porazek.
 */

namespace EterBase {

// ============================================================================
// Standardowe Typy Wyliczeniowe Bledow Domenowych
// ============================================================================

enum class PacketError : uint8_t {
    None = 0,
    BufferUnderflow,
    InvalidHeader,
    UnknownOpcode,
    MalformedPayload,
    ChecksumMismatch,
    SessionClosed,
    SequenceMismatch
};

enum class EntityError : uint8_t {
    None = 0,
    NotFound,
    AlreadyExists,
    InvalidType,
    Dead,
    OutOfRange
};

enum class InventoryError : uint8_t {
    None = 0,
    SlotOutOfRange,
    SlotEmpty,
    SlotOccupied,
    InvalidVnum,
    InsufficientCount,
    ItemLocked
};

enum class CombatError : uint8_t {
    None = 0,
    TargetNotFound,
    TargetDead,
    OutOfRange,
    OnCooldown,
    InvalidAction
};

enum class NavigationError : uint8_t {
    None = 0,
    PathNotFound,
    BlockedTerrain,
    MapNotLoaded,
    DestinationUnreachable
};

// ============================================================================
// Konwersje Bledow na Czytelny Tekst (dla Logowania i Diagnostyki AI)
// ============================================================================

[[nodiscard]] constexpr std::string_view ToString(PacketError err) noexcept {
    switch (err) {
        case PacketError::None: return "None";
        case PacketError::BufferUnderflow: return "BufferUnderflow";
        case PacketError::InvalidHeader: return "InvalidHeader";
        case PacketError::UnknownOpcode: return "UnknownOpcode";
        case PacketError::MalformedPayload: return "MalformedPayload";
        case PacketError::ChecksumMismatch: return "ChecksumMismatch";
        case PacketError::SessionClosed: return "SessionClosed";
        case PacketError::SequenceMismatch: return "SequenceMismatch";
    }
    return "UnknownPacketError";
}

[[nodiscard]] constexpr std::string_view ToString(EntityError err) noexcept {
    switch (err) {
        case EntityError::None: return "None";
        case EntityError::NotFound: return "NotFound";
        case EntityError::AlreadyExists: return "AlreadyExists";
        case EntityError::InvalidType: return "InvalidType";
        case EntityError::Dead: return "Dead";
        case EntityError::OutOfRange: return "OutOfRange";
    }
    return "UnknownEntityError";
}

[[nodiscard]] constexpr std::string_view ToString(InventoryError err) noexcept {
    switch (err) {
        case InventoryError::None: return "None";
        case InventoryError::SlotOutOfRange: return "SlotOutOfRange";
        case InventoryError::SlotEmpty: return "SlotEmpty";
        case InventoryError::SlotOccupied: return "SlotOccupied";
        case InventoryError::InvalidVnum: return "InvalidVnum";
        case InventoryError::InsufficientCount: return "InsufficientCount";
        case InventoryError::ItemLocked: return "ItemLocked";
    }
    return "UnknownInventoryError";
}

// ============================================================================
// Uniwersalne Aliasy i Pomocniki Result C++23
// ============================================================================

/// @brief Podstawowy szablon rezultatu zwracajacego wartosc typu T lub blad typu E
template <typename T, typename E = std::string_view>
using Result = std::expected<T, E>;

/// @brief Rezultat operacji bezwartosciowej (void) zwracajacy sukces lub blad
template <typename E = std::string_view>
using VoidResult = std::expected<void, E>;

/// @brief Dedykowany rezultat parsowania i obslugi pakietow sieciowych
template <typename T = void>
using PacketResult = std::expected<T, PacketError>;

/// @brief Pomocnik tworzenia bledu unexpected dla zwiezlosci w C++23
template <typename E>
[[nodiscard]] constexpr auto MakeError(E&& error) {
    return std::unexpected(std::forward<E>(error));
}

} // namespace EterBase

// ============================================================================
// Specjalizacje std::formatter dla logowania bledow w std::format
// ============================================================================

template <>
struct std::formatter<EterBase::PacketError> : std::formatter<std::string_view> {
    auto format(EterBase::PacketError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(EterBase::ToString(err), ctx);
    }
};

template <>
struct std::formatter<EterBase::EntityError> : std::formatter<std::string_view> {
    auto format(EterBase::EntityError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(EterBase::ToString(err), ctx);
    }
};

template <>
struct std::formatter<EterBase::InventoryError> : std::formatter<std::string_view> {
    auto format(EterBase::InventoryError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(EterBase::ToString(err), ctx);
    }
};
