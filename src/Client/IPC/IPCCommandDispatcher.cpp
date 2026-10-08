#include "IPCCommandDispatcher.h"
#include <cmath>

namespace Client::IPC {

IPCCommandDispatcher::IPCCommandDispatcher(::Core::EventBus& eventBus, Client::Core::GameSession* gameSession)
    : m_eventBus(eventBus), m_gameSession(gameSession) {
}

std::expected<void, IpcDispatchError> IPCCommandDispatcher::Dispatch(const DecodedCommand& cmd) {
    switch (cmd.opcode) {
        case IpcOpcode::Goto:
            if (auto* p = std::get_if<IpcGotoPayload>(&cmd.payload))
                return HandleGoto(*p);
            return std::unexpected(IpcDispatchError::UnknownOpcode);
        case IpcOpcode::AttackTarget:
            if (auto* p = std::get_if<IpcAttackPayload>(&cmd.payload))
                return HandleAttackTarget(*p);
            return std::unexpected(IpcDispatchError::UnknownOpcode);
        case IpcOpcode::PickupLoot:
            if (auto* p = std::get_if<IpcPickupPayload>(&cmd.payload))
                return HandlePickupLoot(*p);
            return std::unexpected(IpcDispatchError::UnknownOpcode);
        case IpcOpcode::UseSkill:
            if (auto* p = std::get_if<IpcUseSkillPayload>(&cmd.payload))
                return HandleUseSkill(*p);
            return std::unexpected(IpcDispatchError::UnknownOpcode);
        case IpcOpcode::UseItem:
            if (auto* p = std::get_if<IpcUseItemPayload>(&cmd.payload))
                return HandleUseItem(*p);
            return std::unexpected(IpcDispatchError::UnknownOpcode);
        case IpcOpcode::SelectTarget:
            if (auto* p = std::get_if<IpcSelectTargetPayload>(&cmd.payload))
                return HandleSelectTarget(*p);
            return std::unexpected(IpcDispatchError::UnknownOpcode);
        default:
            return std::unexpected(IpcDispatchError::UnknownOpcode);
    }
}

std::expected<void, IpcDispatchError> IPCCommandDispatcher::HandleGoto(const IpcGotoPayload& payload) {
    if (std::isnan(payload.destination.x) || std::isnan(payload.destination.y) || std::isnan(payload.destination.z)) {
        return std::unexpected(IpcDispatchError::InvalidCoordinates);
    }
    
    // For SelectTarget, maybe we should publish to EventBus, but here we can just use GameSession.
    if (m_gameSession) {
        Core::MoveCommand cmd{};
        cmd.destination = payload.destination;
        cmd.rotation = 0.0f; // Could be computed or provided later
        cmd.moveType = 1;    // 1 = Run
        cmd.clientTimestamp = 0;
        
        if (!m_gameSession->Execute(cmd)) {
            return std::unexpected(IpcDispatchError::ExecutionFailed);
        }
    }
    
    // Also publish event just in case UI wants to react
    // But no existing GotoEvent in EventBus.h is needed per prompt unless it's the IpcGotoEvent from IPCCommandListenerLite.
    
    return {};
}

std::expected<void, IpcDispatchError> IPCCommandDispatcher::HandleAttackTarget(const IpcAttackPayload& payload) {
    if (payload.targetVid.get() == 0) {
        return std::unexpected(IpcDispatchError::ZeroVid);
    }

    if (m_gameSession) {
        Core::AttackCommand cmd{};
        cmd.targetVid = payload.targetVid;
        cmd.attackType = 0; // Default attack
        
        if (!m_gameSession->Execute(cmd)) {
            return std::unexpected(IpcDispatchError::ExecutionFailed);
        }
    }
    return {};
}

std::expected<void, IpcDispatchError> IPCCommandDispatcher::HandlePickupLoot(const IpcPickupPayload& payload) {
    if (payload.itemVid.get() == 0) {
        return std::unexpected(IpcDispatchError::ZeroVid);
    }

    if (m_gameSession) {
        Core::PickupCommand cmd{};
        cmd.itemVid = payload.itemVid;
        
        if (!m_gameSession->Execute(cmd)) {
            return std::unexpected(IpcDispatchError::ExecutionFailed);
        }
    }
    return {};
}

std::expected<void, IpcDispatchError> IPCCommandDispatcher::HandleUseSkill(const IpcUseSkillPayload& payload) {
    // Note: SkillId 0 might be valid (e.g., standard attack) but targetVid = 0 usually isn't, though some skills don't need a target.
    // Assuming 0 is invalid for target if it targets. Let's just say we don't block target 0 because it could be self buff.
    if (m_gameSession) {
        Core::UseSkillCommand cmd{};
        cmd.skillId = payload.skillId;
        cmd.targetVid = payload.targetVid;
        
        if (!m_gameSession->Execute(cmd)) {
            return std::unexpected(IpcDispatchError::ExecutionFailed);
        }
    }
    return {};
}

std::expected<void, IpcDispatchError> IPCCommandDispatcher::HandleUseItem(const IpcUseItemPayload& payload) {
    if (payload.slot.get() == 0xFFFF) {
        return std::unexpected(IpcDispatchError::InvalidSlot);
    }

    if (m_gameSession) {
        Core::UseItemCommand cmd{};
        cmd.slot = payload.slot;
        
        if (!m_gameSession->Execute(cmd)) {
            return std::unexpected(IpcDispatchError::ExecutionFailed);
        }
    }
    return {};
}

std::expected<void, IpcDispatchError> IPCCommandDispatcher::HandleSelectTarget(const IpcSelectTargetPayload& payload) {
    if (payload.targetVid.get() == 0) {
        return std::unexpected(IpcDispatchError::ZeroVid);
    }

    // Publish event for target selection on EventBus
    m_eventBus.Publish(UserInterface::Core::TargetBoardRefreshEvent(payload.targetVid.get()));
    
    return {};
}

} // namespace Client::IPC
