#pragma once

#include <cstdint>
#include "EventBus.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"

namespace UserInterface::Core::InventoryEvents {

/**
 * @brief Event triggered when a player acquires an item into their inventory.
 */
struct ItemAcquired : public IEvent {
    EterBase::ItemVnum vnum; ///< The virtual number of the acquired item.
    EterBase::ItemSlot slot; ///< The inventory slot where the item was placed.
    uint32_t count;          ///< The quantity of the acquired item.

    /**
     * @brief Constructs an ItemAcquired event.
     * @param vnum The virtual number of the acquired item.
     * @param slot The inventory slot where the item was placed.
     * @param count The quantity of the acquired item.
     */
    ItemAcquired(EterBase::ItemVnum vnum, EterBase::ItemSlot slot, uint32_t count)
        : vnum(vnum), slot(slot), count(count) {}
};

/**
 * @brief Event triggered when a player equips an item.
 */
struct ItemEquipped : public IEvent {
    EterBase::ItemVnum vnum; ///< The virtual number of the equipped item.
    EterBase::ItemSlot slot; ///< The equipment slot where the item is equipped.

    /**
     * @brief Constructs an ItemEquipped event.
     * @param vnum The virtual number of the equipped item.
     * @param slot The equipment slot where the item is equipped.
     */
    ItemEquipped(EterBase::ItemVnum vnum, EterBase::ItemSlot slot)
        : vnum(vnum), slot(slot) {}
};

/**
 * @brief Event triggered when a player drops an item from their inventory.
 */
struct ItemDropped : public IEvent {
    EterBase::ItemVnum vnum; ///< The virtual number of the dropped item.
    EterBase::ItemSlot slot; ///< The inventory slot the item was dropped from.
    uint32_t count;          ///< The quantity of the dropped item.

    /**
     * @brief Constructs an ItemDropped event.
     * @param vnum The virtual number of the dropped item.
     * @param slot The inventory slot the item was dropped from.
     * @param count The quantity of the dropped item.
     */
    ItemDropped(EterBase::ItemVnum vnum, EterBase::ItemSlot slot, uint32_t count)
        : vnum(vnum), slot(slot), count(count) {}
};

/**
 * @brief Event triggered when the player's gold amount changes.
 */
struct GoldChanged : public IEvent {
    uint64_t amount; ///< The total amount of gold after the change.
    int64_t delta;   ///< The difference in gold (+ for gain, - for loss).

    /**
     * @brief Constructs a GoldChanged event.
     * @param amount The total amount of gold after the change.
     * @param delta The difference in gold.
     */
    GoldChanged(uint64_t amount, int64_t delta)
        : amount(amount), delta(delta) {}
};

} // namespace UserInterface::Core::InventoryEvents
