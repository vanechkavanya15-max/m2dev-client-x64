#pragma once

#include <cstdint>
#include <string_view>
#include <format>
#include <expected>

namespace Client::Core {

/**
 * @brief Enumy bledow domenowych operacji ekwipunku.
 */
enum class InventoryError : uint8_t {
    None = 0,
    SlotOutOfBounds,
    SlotOccupied,
    SlotEmpty,
    ItemLocked,
    InsufficientCount,
    InvalidVnum,
    SlotOutOfRange = SlotOutOfBounds
};

/**
 * @brief Enumy bledow domenowych operacji umiejetnosci.
 */
enum class SkillError : uint8_t {
    None = 0,
    SkillNotFound,
    NotEnoughSP,
    OnCooldown,
    RequirementNotMet
};

/**
 * @brief Enumy bledow domenowych operacji na aktorach/jednostkach.
 */
enum class ActorError : uint8_t {
    None = 0,
    ActorNotFound,
    AlreadyDead,
    InvalidPosition
};

// Nowoczesny szablon Result oparty na std::expected
template <typename T, typename E>
using Result = std::expected<T, E>;

[[nodiscard]] inline std::string_view to_string(InventoryError error) noexcept {
    switch (error) {
        case InventoryError::None: return "InventoryError::None";
        case InventoryError::SlotOutOfBounds: return "InventoryError::SlotOutOfBounds";
        case InventoryError::SlotOccupied: return "InventoryError::SlotOccupied - The target slot is already occupied";
        case InventoryError::SlotEmpty: return "InventoryError::SlotEmpty - The target slot contains no item";
        case InventoryError::ItemLocked: return "InventoryError::ItemLocked - The item is locked";
        case InventoryError::InsufficientCount: return "InventoryError::InsufficientCount - Not enough item count to perform operation";
        case InventoryError::InvalidVnum: return "InventoryError::InvalidVnum - The item VNUM is invalid";
        default: return "InventoryError::Unknown";
    }
}

[[nodiscard]] inline std::string_view to_string(SkillError error) noexcept {
    switch (error) {
        case SkillError::None: return "SkillError::None";
        case SkillError::SkillNotFound: return "SkillError::SkillNotFound";
        case SkillError::NotEnoughSP: return "SkillError::NotEnoughSP";
        case SkillError::OnCooldown: return "SkillError::OnCooldown";
        case SkillError::RequirementNotMet: return "SkillError::RequirementNotMet";
        default: return "SkillError::Unknown";
    }
}

[[nodiscard]] inline std::string_view to_string(ActorError error) noexcept {
    switch (error) {
        case ActorError::None: return "ActorError::None";
        case ActorError::ActorNotFound: return "ActorError::ActorNotFound";
        case ActorError::AlreadyDead: return "ActorError::AlreadyDead";
        case ActorError::InvalidPosition: return "ActorError::InvalidPosition";
        default: return "ActorError::Unknown";
    }
}

} // namespace Client::Core

namespace Core {
    using InventoryError = ::Client::Core::InventoryError;
    using SkillError = ::Client::Core::SkillError;
    using ActorError = ::Client::Core::ActorError;
    template <typename T, typename E>
    using Result = ::Client::Core::Result<T, E>;
}

template <>
struct std::formatter<Client::Core::InventoryError> : std::formatter<std::string_view> {
    auto format(Client::Core::InventoryError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Core::to_string(err), ctx);
    }
};

template <>
struct std::formatter<Client::Core::SkillError> : std::formatter<std::string_view> {
    auto format(Client::Core::SkillError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Core::to_string(err), ctx);
    }
};

template <>
struct std::formatter<Client::Core::ActorError> : std::formatter<std::string_view> {
    auto format(Client::Core::ActorError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Core::to_string(err), ctx);
    }
};
