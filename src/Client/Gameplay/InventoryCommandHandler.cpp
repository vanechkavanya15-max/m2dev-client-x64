#include "InventoryCommandHandler.h"
#include "InventoryDomain.h"

namespace Client::Gameplay {

InventoryCommandHandler::InventoryCommandHandler(InventoryDomain& inventoryDomain, const IPlayerConditionProvider& conditionProvider)
    : m_inventoryDomain(inventoryDomain), m_conditionProvider(conditionProvider) {}

EterBase::Result<void, CommandError> InventoryCommandHandler::CheckCommonConditions(uint8_t windowType, EterBase::ItemSlot slot) const {
    if (m_conditionProvider.IsDead()) {
        return std::unexpected(CommandError::PlayerDead);
    }
    if (m_conditionProvider.IsTrading()) {
        return std::unexpected(CommandError::PlayerTrading);
    }

    auto itemRes = m_inventoryDomain.GetItem(windowType, slot);
    if (!itemRes.has_value()) {
        return std::unexpected(CommandError::SlotEmpty);
    }

    return {};
}

EterBase::Result<void, CommandError> InventoryCommandHandler::Handle(const InventoryCommand& command) {
    return std::visit([this](auto&& cmd) -> EterBase::Result<void, CommandError> {
        using T = std::decay_t<decltype(cmd)>;
        if constexpr (std::is_same_v<T, UseItemCommand>) {
            return HandleUse(cmd);
        } else if constexpr (std::is_same_v<T, DropItemCommand>) {
            return HandleDrop(cmd);
        } else if constexpr (std::is_same_v<T, MoveItemCommand>) {
            return HandleMove(cmd);
        } else {
            return std::unexpected(CommandError::InvalidCommand);
        }
    }, command);
}

EterBase::Result<void, CommandError> InventoryCommandHandler::HandleUse(const UseItemCommand& cmd) {
    auto commonCheck = CheckCommonConditions(cmd.windowType, cmd.slot);
    if (!commonCheck.has_value()) {
        return commonCheck;
    }

    // Typical use item logic... omitted for this specific requirement focusing on domain checks
    
    return {};
}

EterBase::Result<void, CommandError> InventoryCommandHandler::HandleDrop(const DropItemCommand& cmd) {
    auto commonCheck = CheckCommonConditions(cmd.windowType, cmd.slot);
    if (!commonCheck.has_value()) {
        return commonCheck;
    }

    auto removeRes = m_inventoryDomain.RemoveItem(cmd.windowType, cmd.slot);
    if (!removeRes.has_value()) {
        return std::unexpected(CommandError::InvalidCommand); // or generic error
    }

    return {};
}

EterBase::Result<void, CommandError> InventoryCommandHandler::HandleMove(const MoveItemCommand& cmd) {
    auto commonCheck = CheckCommonConditions(cmd.srcWindowType, cmd.srcSlot);
    if (!commonCheck.has_value()) {
        return commonCheck;
    }
    
    // Only verify destination windowType isn't restricted by trade or death, not if it's empty
    // since SwapItem can handle swaps
    
    auto swapRes = m_inventoryDomain.SwapItem(cmd.srcWindowType, cmd.srcSlot, cmd.dstWindowType, cmd.dstSlot);
    if (!swapRes.has_value()) {
        return std::unexpected(CommandError::InvalidCommand);
    }

    return {};
}

} // namespace Client::Gameplay
