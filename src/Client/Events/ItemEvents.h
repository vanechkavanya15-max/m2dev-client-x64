#pragma once

#include <cstdint>
#include <optional>

#include "EterBase/EventBus.h"
#include "Client/Gameplay/InventoryDomain.h"

namespace Client::Events {

/**
 * @brief Zdarzenie domenowe wystepujace podczas dodawania przedmiotu do inwentarza.
 */
struct ItemAddedEvent : public EterBase::IEvent {
    Client::Gameplay::InventoryWindow windowType;
    Client::Gameplay::SlotIndex slot;
    Client::Gameplay::ItemData itemData;

    constexpr ItemAddedEvent(Client::Gameplay::InventoryWindow windowType, Client::Gameplay::SlotIndex slot, const Client::Gameplay::ItemData& itemData)
        : windowType(windowType), slot(slot), itemData(itemData) {}

    constexpr bool operator==(const ItemAddedEvent& other) const {
        return windowType == other.windowType && slot == other.slot && itemData == other.itemData;
    }
};

/**
 * @brief Zdarzenie domenowe wystepujace podczas usuniecia przedmiotu z inwentarza.
 */
struct ItemRemovedEvent : public EterBase::IEvent {
    Client::Gameplay::InventoryWindow windowType;
    Client::Gameplay::SlotIndex slot;
    Client::Gameplay::ItemData removedItem; // Przechowuje informacje o tym, co zostalo usuniete

    constexpr ItemRemovedEvent(Client::Gameplay::InventoryWindow windowType, Client::Gameplay::SlotIndex slot, const Client::Gameplay::ItemData& removedItem)
        : windowType(windowType), slot(slot), removedItem(removedItem) {}

    constexpr bool operator==(const ItemRemovedEvent& other) const {
        return windowType == other.windowType && slot == other.slot && removedItem == other.removedItem;
    }
};

/**
 * @brief Zdarzenie domenowe wystepujace podczas przenoszenia/zamiany przedmiotu.
 */
struct ItemMovedEvent : public EterBase::IEvent {
    Client::Gameplay::InventoryWindow sourceWindow;
    Client::Gameplay::SlotIndex sourceSlot;
    
    Client::Gameplay::InventoryWindow targetWindow;
    Client::Gameplay::SlotIndex targetSlot;

    Client::Gameplay::ItemData movedItem;

    constexpr ItemMovedEvent(
        Client::Gameplay::InventoryWindow sourceWindow, Client::Gameplay::SlotIndex sourceSlot,
        Client::Gameplay::InventoryWindow targetWindow, Client::Gameplay::SlotIndex targetSlot,
        const Client::Gameplay::ItemData& movedItem)
        : sourceWindow(sourceWindow), sourceSlot(sourceSlot),
          targetWindow(targetWindow), targetSlot(targetSlot),
          movedItem(movedItem) {}

    constexpr bool operator==(const ItemMovedEvent& other) const {
        return sourceWindow == other.sourceWindow && sourceSlot == other.sourceSlot &&
               targetWindow == other.targetWindow && targetSlot == other.targetSlot &&
               movedItem == other.movedItem;
    }
};

/**
 * @brief Zdarzenie domenowe wystepujace podczas zmiany wyposazenia pancerza (lub calego ekwipunku).
 * Zbroja, bron itp. 
 */
struct ArmorEquippedEvent : public EterBase::IEvent {
    Client::Gameplay::SlotIndex equipmentSlot; // np. Slot odpowiadajacy za zbroje w oknie Equipment
    std::optional<Client::Gameplay::ItemData> oldArmor;
    std::optional<Client::Gameplay::ItemData> newArmor;

    constexpr ArmorEquippedEvent(
        Client::Gameplay::SlotIndex equipmentSlot,
        std::optional<Client::Gameplay::ItemData> oldArmor,
        std::optional<Client::Gameplay::ItemData> newArmor)
        : equipmentSlot(equipmentSlot), oldArmor(oldArmor), newArmor(newArmor) {}

    constexpr bool operator==(const ArmorEquippedEvent& other) const {
        return equipmentSlot == other.equipmentSlot && oldArmor == other.oldArmor && newArmor == other.newArmor;
    }
};

} // namespace Client::Events
