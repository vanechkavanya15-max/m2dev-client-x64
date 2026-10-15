#pragma once

#include <cstdint>

namespace Client::Core {

/**
 * @brief Zdarzenie domenowe aktualizacji pojedynczego slotu w ekwipunku.
 */
struct InventorySlotUpdatedEvent {
    uint16_t slot{0};
    uint32_t vnum{0};
    uint32_t count{0};

    constexpr InventorySlotUpdatedEvent() = default;
    constexpr InventorySlotUpdatedEvent(uint16_t slot, uint32_t vnum, uint32_t count)
        : slot(slot), vnum(vnum), count(count) {}

    constexpr bool operator==(const InventorySlotUpdatedEvent& other) const = default;
};

/**
 * @brief Zdarzenie domenowe rozpoczecia czasu odnawiania umiejetnosci.
 */
struct SkillCooldownStartedEvent {
    uint32_t skillId{0};
    uint32_t durationMs{0};

    constexpr SkillCooldownStartedEvent() = default;
    constexpr SkillCooldownStartedEvent(uint32_t skillId, uint32_t durationMs)
        : skillId(skillId), durationMs(durationMs) {}

    constexpr bool operator==(const SkillCooldownStartedEvent& other) const = default;
};

/**
 * @brief Zdarzenie domenowe zgonu aktora w swiecie gry.
 */
struct ActorDeadEvent {
    uint32_t vid{0};

    constexpr ActorDeadEvent() = default;
    constexpr explicit ActorDeadEvent(uint32_t vid) : vid(vid) {}

    constexpr bool operator==(const ActorDeadEvent& other) const = default;
};

/**
 * @brief Zdarzenie domenowe aktualizacji stanu zlota postaci.
 */
struct PlayerGoldUpdatedEvent {
    int64_t oldGold{0};
    int64_t newGold{0};

    constexpr PlayerGoldUpdatedEvent() = default;
    constexpr PlayerGoldUpdatedEvent(int64_t oldGold, int64_t newGold)
        : oldGold(oldGold), newGold(newGold) {}

    constexpr bool operator==(const PlayerGoldUpdatedEvent& other) const = default;
};

} // namespace Client::Core

namespace Core {
    using InventorySlotUpdatedEvent = ::Client::Core::InventorySlotUpdatedEvent;
    using SkillCooldownStartedEvent = ::Client::Core::SkillCooldownStartedEvent;
    using PlayerGoldUpdatedEvent = ::Client::Core::PlayerGoldUpdatedEvent;
}

