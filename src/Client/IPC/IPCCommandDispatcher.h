#pragma once

#include <cstdint>
#include <expected>
#include <variant>
#include <string_view>
#include <format>
#include "Client/Core/Result.h"
#include "Client/Core/StrongTypes.h"
#include "Client/Core/GameSession.h"
#include "UserInterface/Core/EventBus.h"

namespace Client::IPC {

// ============================================================================
// IPC Opcodes and Errors
// ============================================================================

enum class IpcOpcode : uint16_t {
    Goto,
    AttackTarget,
    PickupLoot,
    UseSkill,
    UseItem,
    SelectTarget
};

enum class IpcDispatchError : uint8_t {
    InvalidCoordinates,
    ZeroVid,
    InvalidSlot,
    UnknownOpcode,
    ExecutionFailed
};

[[nodiscard]] constexpr std::string_view to_string(IpcDispatchError err) noexcept {
    switch (err) {
        case IpcDispatchError::InvalidCoordinates: return "InvalidCoordinates";
        case IpcDispatchError::ZeroVid: return "ZeroVid";
        case IpcDispatchError::InvalidSlot: return "InvalidSlot";
        case IpcDispatchError::UnknownOpcode: return "UnknownOpcode";
        case IpcDispatchError::ExecutionFailed: return "ExecutionFailed";
    }
    return "UnknownError";
}

// ============================================================================
// Decoded Commands
// ============================================================================

struct IpcGotoPayload {
    Core::MapCoords destination;
};

struct IpcAttackPayload {
    Core::EntityVid targetVid;
};

struct IpcPickupPayload {
    Core::EntityVid itemVid;
};

struct IpcUseSkillPayload {
    Core::SkillId skillId;
    Core::EntityVid targetVid;
};

struct IpcUseItemPayload {
    Core::ItemSlot slot;
};

struct IpcSelectTargetPayload {
    Core::EntityVid targetVid;
};

struct DecodedCommand {
    IpcOpcode opcode;
    std::variant<
        IpcGotoPayload,
        IpcAttackPayload,
        IpcPickupPayload,
        IpcUseSkillPayload,
        IpcUseItemPayload,
        IpcSelectTargetPayload
    > payload;
};

// ============================================================================
// IPC Command Dispatcher
// ============================================================================

class IPCCommandDispatcher {
public:
    explicit IPCCommandDispatcher(::Core::EventBus& eventBus, Client::Core::GameSession* gameSession = nullptr);
    ~IPCCommandDispatcher() = default;

    IPCCommandDispatcher(const IPCCommandDispatcher&) = delete;
    IPCCommandDispatcher& operator=(const IPCCommandDispatcher&) = delete;

    [[nodiscard]] std::expected<void, IpcDispatchError> Dispatch(const DecodedCommand& cmd);

private:
    ::Core::EventBus& m_eventBus;
    Client::Core::GameSession* m_gameSession;

    std::expected<void, IpcDispatchError> HandleGoto(const IpcGotoPayload& payload);
    std::expected<void, IpcDispatchError> HandleAttackTarget(const IpcAttackPayload& payload);
    std::expected<void, IpcDispatchError> HandlePickupLoot(const IpcPickupPayload& payload);
    std::expected<void, IpcDispatchError> HandleUseSkill(const IpcUseSkillPayload& payload);
    std::expected<void, IpcDispatchError> HandleUseItem(const IpcUseItemPayload& payload);
    std::expected<void, IpcDispatchError> HandleSelectTarget(const IpcSelectTargetPayload& payload);
};

} // namespace Client::IPC

// Formatter specialization
template <>
struct std::formatter<Client::IPC::IpcDispatchError> : std::formatter<std::string_view> {
    auto format(Client::IPC::IpcDispatchError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::IPC::to_string(err), ctx);
    }
};
