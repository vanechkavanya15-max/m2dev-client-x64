#pragma once

#include <cstdint>
#include <string_view>
#include <variant>
#include <format>
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"

namespace Client::Gameplay {

class InventoryDomain;

enum class CommandError : uint8_t {
    None = 0,
    PlayerDead,
    PlayerTrading,
    SlotEmpty,
    InvalidCommand
};

[[nodiscard]] constexpr std::string_view ToString(CommandError err) noexcept {
    switch (err) {
        case CommandError::None: return "None";
        case CommandError::PlayerDead: return "PlayerDead";
        case CommandError::PlayerTrading: return "PlayerTrading";
        case CommandError::SlotEmpty: return "SlotEmpty";
        case CommandError::InvalidCommand: return "InvalidCommand";
    }
    return "UnknownCommandError";
}

struct UseItemCommand {
    uint8_t windowType;
    EterBase::ItemSlot slot;
};

struct DropItemCommand {
    uint8_t windowType;
    EterBase::ItemSlot slot;
};

struct MoveItemCommand {
    uint8_t srcWindowType;
    EterBase::ItemSlot srcSlot;
    uint8_t dstWindowType;
    EterBase::ItemSlot dstSlot;
};

using InventoryCommand = std::variant<UseItemCommand, DropItemCommand, MoveItemCommand>;

class IPlayerConditionProvider {
public:
    virtual ~IPlayerConditionProvider() = default;
    [[nodiscard]] virtual bool IsDead() const = 0;
    [[nodiscard]] virtual bool IsTrading() const = 0;
};

class InventoryCommandHandler {
public:
    InventoryCommandHandler(InventoryDomain& inventoryDomain, const IPlayerConditionProvider& conditionProvider);

    EterBase::Result<void, CommandError> Handle(const InventoryCommand& command);

private:
    EterBase::Result<void, CommandError> CheckCommonConditions(uint8_t windowType, EterBase::ItemSlot slot) const;

    EterBase::Result<void, CommandError> HandleUse(const UseItemCommand& cmd);
    EterBase::Result<void, CommandError> HandleDrop(const DropItemCommand& cmd);
    EterBase::Result<void, CommandError> HandleMove(const MoveItemCommand& cmd);

    InventoryDomain& m_inventoryDomain;
    const IPlayerConditionProvider& m_conditionProvider;
};

} // namespace Client::Gameplay

template <>
struct std::formatter<Client::Gameplay::CommandError> : std::formatter<std::string_view> {
    auto format(Client::Gameplay::CommandError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Gameplay::ToString(err), ctx);
    }
};
