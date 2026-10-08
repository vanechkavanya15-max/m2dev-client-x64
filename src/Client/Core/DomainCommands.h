#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include "StrongTypes.h"

namespace Client::Core {

// ============================================================================
// Błędy wykonania komend domenowych
// ============================================================================
enum class CommandError : uint8_t {
    InvalidTarget,
    OutOfRange,
    CooldownActive,
    InsufficientSp,
    InsufficientStamina,
    SlotEmpty,
    InventoryFull,
    MovementBlocked,
    Disconnected,
    RateLimited,
    InvalidParameter
};

[[nodiscard]] inline std::string_view to_string(CommandError err) noexcept {
    switch (err) {
        case CommandError::InvalidTarget:       return "InvalidTarget";
        case CommandError::OutOfRange:          return "OutOfRange";
        case CommandError::CooldownActive:      return "CooldownActive";
        case CommandError::InsufficientSp:      return "InsufficientSp";
        case CommandError::InsufficientStamina: return "InsufficientStamina";
        case CommandError::SlotEmpty:           return "SlotEmpty";
        case CommandError::InventoryFull:       return "InventoryFull";
        case CommandError::MovementBlocked:     return "MovementBlocked";
        case CommandError::Disconnected:        return "Disconnected";
        case CommandError::RateLimited:         return "RateLimited";
        case CommandError::InvalidParameter:    return "InvalidParameter";
        default:                                return "UnknownCommandError";
    }
}

// ============================================================================
// Deklaratywne Komendy Domenowe (Zero-memcpy, High-level Actions)
// ============================================================================

struct AttackCommand {
    EntityVid targetVid{0};
    uint8_t attackType{0};
    std::optional<float> targetRotation;
};

struct ShootCommand {
    EntityVid targetVid{0};
    uint8_t skillVnum{0};
};

struct MoveCommand {
    MapCoords destination{};
    float rotation{0.0f};
    uint8_t moveType{0}; // 0 = Walk, 1 = Run
    uint32_t clientTimestamp{0};
};

struct SyncPositionCommand {
    EntityVid vid{0};
    MapCoords coords{};
};

struct UseSkillCommand {
    SkillId skillId{0};
    EntityVid targetVid{0};
};

struct UseItemCommand {
    ItemSlot slot{0xFFFF};
};

struct DropItemCommand {
    ItemSlot slot{0xFFFF};
    uint32_t count{1};
};

struct MoveItemCommand {
    ItemSlot sourceSlot{0xFFFF};
    ItemSlot targetSlot{0xFFFF};
    uint32_t count{1};
};

struct PickupCommand {
    EntityVid itemVid{0};
};

struct InteractNpcCommand {
    EntityVid npcVid{0};
    uint8_t actionType{0};
};

struct ChatCommand {
    std::string message;
    uint8_t chatType{0}; // Talk, Party, Guild, Shout
};

struct WhisperCommand {
    std::string recipientName;
    std::string message;
};

struct ShopBuyCommand {
    uint8_t shopSlot{0};
    uint32_t count{1};
};

struct ShopSellCommand {
    ItemSlot inventorySlot{0xFFFF};
    uint32_t count{1};
};

struct QuestAnswerCommand {
    uint8_t answerIndex{0};
};

struct QuestConfirmCommand {
    uint8_t answer{0};
    uint32_t requestPID{0};
};

} // namespace Client::Core
