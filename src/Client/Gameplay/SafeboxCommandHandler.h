#pragma once

#include <string_view>
#include <format>
#include <cstdint>
#include <expected>

#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "InventoryDomain.h"
#include "TradeDomain.h"

namespace Client::Gameplay {

enum class CommandError : uint8_t {
    None = 0,
    InvalidPassword,
    ItemNotFound,
    InventoryFull,
    SafeboxFull,
    InvalidSlot,
    NotOpened,
    AlreadyOpened
};

[[nodiscard]] constexpr std::string_view ToString(CommandError err) noexcept {
    switch (err) {
        case CommandError::None: return "None";
        case CommandError::InvalidPassword: return "InvalidPassword";
        case CommandError::ItemNotFound: return "ItemNotFound";
        case CommandError::InventoryFull: return "InventoryFull";
        case CommandError::SafeboxFull: return "SafeboxFull";
        case CommandError::InvalidSlot: return "InvalidSlot";
        case CommandError::NotOpened: return "NotOpened";
        case CommandError::AlreadyOpened: return "AlreadyOpened";
    }
    return "UnknownCommandError";
}

class SafeboxCommandHandler {
public:
    SafeboxCommandHandler(InventoryDomain& inventory, SafeBox& safebox);

    EterBase::Result<void, CommandError> ValidatePassword(std::string_view password);
    EterBase::Result<void, CommandError> MoveItemToSafebox(EterBase::ItemSlot inventorySlot, EterBase::ItemSlot safeboxSlot);
    EterBase::Result<void, CommandError> MoveItemToInventory(EterBase::ItemSlot safeboxSlot, EterBase::ItemSlot inventorySlot);
    EterBase::Result<void, CommandError> Close();

    bool IsOpen() const;

private:
    InventoryDomain& m_inventory;
    SafeBox& m_safebox;
    bool m_isOpen{false};
};

} // namespace Client::Gameplay

template <>
struct std::formatter<Client::Gameplay::CommandError> : std::formatter<std::string_view> {
    auto format(Client::Gameplay::CommandError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Gameplay::ToString(err), ctx);
    }
};
