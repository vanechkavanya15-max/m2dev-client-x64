#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <format>
#include <cstdint>
#include "DomainErrors.h"

namespace Client::Core {

// Domenowe enumy błędu
enum class PacketError : uint8_t {
    BufferUnderflow,
    InvalidHeader,
    ChecksumMismatch,
    VidMismatch,
    Timeout
};

enum class EntityError : uint8_t {
    NotFound,
    AlreadyExists,
    Dead,
    OutOfRange
};

enum class MountError : uint8_t {
    NoHorseInstance,
    MotionKeyNotFound,
    InvalidState
};

// Funkcje pomocnicze do konwersji na tekst
[[nodiscard]] std::string_view to_string(PacketError error) noexcept;
[[nodiscard]] std::string_view to_string(EntityError error) noexcept;
[[nodiscard]] std::string_view to_string(MountError error) noexcept;

// Nowoczesny szablon Result oparty na std::expected
template <typename T, typename E>
using Result = std::expected<T, E>;

} // namespace Client::Core

// Specjalizacje std::formatter dla formatowania C++20/23
template <>
struct std::formatter<Client::Core::PacketError> : std::formatter<std::string_view> {
    auto format(Client::Core::PacketError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Core::to_string(err), ctx);
    }
};

template <>
struct std::formatter<Client::Core::EntityError> : std::formatter<std::string_view> {
    auto format(Client::Core::EntityError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Core::to_string(err), ctx);
    }
};

template <>
struct std::formatter<Client::Core::MountError> : std::formatter<std::string_view> {
    auto format(Client::Core::MountError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Core::to_string(err), ctx);
    }
};
