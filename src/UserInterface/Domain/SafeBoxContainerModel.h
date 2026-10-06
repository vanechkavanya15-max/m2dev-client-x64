#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <functional>
#include <span>
#include <stdexcept>

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace UserInterface::Domain {

/**
 * @brief Represents a single item attribute/bonus in the SafeBox.
 */
struct SafeBoxItemAttribute {
    uint16_t type;  ///< Type of the attribute
    int16_t value;  ///< Value of the attribute
};

/**
 * @brief Core data structure representing an item in the SafeBox or Mall.
 * 
 * Uses C++23 StrongTypes for identifiers and std::vector for flexible properties.
 */
struct SafeBoxItemData {
    EterBase::ItemVnum vnum;                        ///< Virtual number of the item
    uint32_t count;                                 ///< Quantity of the item
    std::vector<uint32_t> sockets;                  ///< Sockets (Metin stones)
    std::vector<SafeBoxItemAttribute> attributes;   ///< Item attributes/bonuses
};

// ============================================================================
// SafeBox Events for EventBus
// ============================================================================

/**
 * @brief Event triggered when the password protection status changes.
 */
struct SafeBoxLockStatusEvent : public Core::IEvent {
    bool isUnlocked;      ///< True if unlocked and ready for use.

    /**
     * @brief Constructs a new SafeBoxLockStatusEvent.
     * @param isUnlocked Lock status.
     */
    explicit SafeBoxLockStatusEvent(bool isUnlocked) : isUnlocked(isUnlocked) {}
};

/**
 * @brief Event triggered when a SafeBox or Mall slot is updated.
 */
struct SafeBoxSlotUpdateEvent : public Core::IEvent {
    bool isMall;          ///< True if the event is for a Mall slot, false for SafeBox
    uint16_t slotIndex;   ///< The index of the updated slot

    /**
     * @brief Constructs a new SafeBoxSlotUpdateEvent.
     * @param isMall Whether the slot is in the Mall.
     * @param slotIndex The index of the slot.
     */
    SafeBoxSlotUpdateEvent(bool isMall, uint16_t slotIndex) 
        : isMall(isMall), slotIndex(slotIndex) {}
};

/**
 * @brief Event triggered when the SafeBox status (open/close, size) changes.
 */
struct SafeBoxStatusEvent : public Core::IEvent {
    bool isOpen;          ///< True if the SafeBox/Mall is now open
    bool isMall;          ///< True if this applies to the Mall
    uint32_t size;        ///< Current size (number of slots)

    /**
     * @brief Constructs a new SafeBoxStatusEvent.
     * @param isOpen Is the container open?
     * @param isMall Is it the Mall?
     * @param size The size of the container.
     */
    SafeBoxStatusEvent(bool isOpen, bool isMall, uint32_t size) 
        : isOpen(isOpen), isMall(isMall), size(size) {}
};

/**
 * @brief Event triggered when the money inside the SafeBox changes.
 */
struct SafeBoxMoneyUpdateEvent : public Core::IEvent {
    uint32_t money;       ///< The new money amount

    /**
     * @brief Constructs a new SafeBoxMoneyUpdateEvent.
     * @param money The updated amount of money.
     */
    explicit SafeBoxMoneyUpdateEvent(uint32_t money) : money(money) {}
};

// ============================================================================
// SafeBox Container Model
// ============================================================================

/**
 * @brief Domain model for managing the SafeBox and Mall (Item Shop storage).
 * 
 * Decoupled from the GUI layer; uses EventBus to broadcast state changes.
 * Adheres to C++23 standards with monadic std::optional operations and StrongTypes.
 */
class SafeBoxContainerModel {
public:
    static constexpr uint16_t SAFEBOX_SLOT_X_COUNT = 5;
    static constexpr uint16_t SAFEBOX_SLOT_Y_COUNT = 9;
    static constexpr uint16_t SAFEBOX_PAGE_SIZE = SAFEBOX_SLOT_X_COUNT * SAFEBOX_SLOT_Y_COUNT;

    /**
     * @brief Constructs the SafeBoxContainerModel.
     */
    SafeBoxContainerModel() : money_(0), safeBoxSize_(0), mallSize_(0), isUnlocked_(false) {}

    /**
     * @brief Unlocks the SafeBox after successful password verification.
     */
    void Unlock() {
        isUnlocked_ = true;
        EterBase::ModernLogger::Info("SafeBoxContainerModel: SafeBox unlocked");
        Core::EventBus::GetInstance().Publish(SafeBoxLockStatusEvent(true));
    }

    /**
     * @brief Locks the SafeBox, requiring password entry again.
     */
    void Lock() {
        isUnlocked_ = false;
        EterBase::ModernLogger::Info("SafeBoxContainerModel: SafeBox locked");
        Core::EventBus::GetInstance().Publish(SafeBoxLockStatusEvent(false));
    }

    /**
     * @brief Checks if the SafeBox is currently unlocked.
     * @return true if unlocked.
     */
    [[nodiscard]] bool IsUnlocked() const {
        return isUnlocked_;
    }

    /**
     * @brief Opens the SafeBox with the given size.
     * @param size The total number of slots in the SafeBox.
     */
    void OpenSafeBox(uint32_t size) {
        safeBoxSize_ = size;
        safeBoxSlots_.resize(size);
        std::fill(safeBoxSlots_.begin(), safeBoxSlots_.end(), std::nullopt);
        
        EterBase::ModernLogger::Info("SafeBoxContainerModel: SafeBox opened with size {}", size);
        Core::EventBus::GetInstance().Publish(SafeBoxStatusEvent(true, false, size));
    }

    /**
     * @brief Closes the SafeBox, clearing all slots.
     */
    void CloseSafeBox() {
        safeBoxSize_ = 0;
        safeBoxSlots_.clear();
        
        EterBase::ModernLogger::Info("SafeBoxContainerModel: SafeBox closed");
        Core::EventBus::GetInstance().Publish(SafeBoxStatusEvent(false, false, 0));
    }

    /**
     * @brief Opens the Mall with the given size.
     * @param size The total number of slots in the Mall.
     */
    void OpenMall(uint32_t size) {
        mallSize_ = size;
        mallSlots_.resize(size);
        std::fill(mallSlots_.begin(), mallSlots_.end(), std::nullopt);
        
        EterBase::ModernLogger::Info("SafeBoxContainerModel: Mall opened with size {}", size);
        Core::EventBus::GetInstance().Publish(SafeBoxStatusEvent(true, true, size));
    }

    /**
     * @brief Sets an item in a specific SafeBox slot.
     * @param slotIndex The index of the slot.
     * @param item The item data to set.
     * @return PacketResult indicating success or PacketError::SequenceMismatch (used generically here for out-of-bounds).
     */
    EterBase::PacketResult<void> SetSafeBoxItem(uint16_t slotIndex, const SafeBoxItemData& item) {
        if (slotIndex >= safeBoxSize_) {
            EterBase::ModernLogger::Warn("SafeBoxContainerModel: Failed to set item. Slot {} out of range", slotIndex);
            return EterBase::MakeError(EterBase::PacketError::SequenceMismatch);
        }

        safeBoxSlots_[slotIndex] = item;
        Core::EventBus::GetInstance().Publish(SafeBoxSlotUpdateEvent(false, slotIndex));
        return {};
    }

    /**
     * @brief Deletes an item from a specific SafeBox slot.
     * @param slotIndex The index of the slot.
     * @return PacketResult indicating success or PacketError::SequenceMismatch if out-of-bounds.
     */
    EterBase::PacketResult<void> DeleteSafeBoxItem(uint16_t slotIndex) {
        if (slotIndex >= safeBoxSize_) {
            return EterBase::MakeError(EterBase::PacketError::SequenceMismatch);
        }

        safeBoxSlots_[slotIndex].reset();
        Core::EventBus::GetInstance().Publish(SafeBoxSlotUpdateEvent(false, slotIndex));
        return {};
    }

    /**
     * @brief Retrieves the item at the specified SafeBox slot.
     * @param slotIndex The index of the slot.
     * @return std::optional containing the item data if present.
     */
    std::optional<SafeBoxItemData> GetSafeBoxItem(uint16_t slotIndex) const {
        if (slotIndex >= safeBoxSize_) {
            return std::nullopt;
        }
        return safeBoxSlots_[slotIndex];
    }

    /**
     * @brief Monadic retrieval of a SafeBox item's Vnum.
     * @param slotIndex The index of the slot.
     * @return std::optional containing the ItemVnum if the item exists.
     */
    std::optional<EterBase::ItemVnum> GetSafeBoxItemVnum(uint16_t slotIndex) const {
        return GetSafeBoxItem(slotIndex).transform([](const SafeBoxItemData& item) {
            return item.vnum;
        });
    }

    /**
     * @brief Sets an item in a specific Mall slot.
     * @param slotIndex The index of the slot.
     * @param item The item data to set.
     * @return PacketResult indicating success or PacketError::SequenceMismatch.
     */
    EterBase::PacketResult<void> SetMallItem(uint16_t slotIndex, const SafeBoxItemData& item) {
        if (slotIndex >= mallSize_) {
            return EterBase::MakeError(EterBase::PacketError::SequenceMismatch);
        }

        mallSlots_[slotIndex] = item;
        Core::EventBus::GetInstance().Publish(SafeBoxSlotUpdateEvent(true, slotIndex));
        return {};
    }

    /**
     * @brief Deletes an item from a specific Mall slot.
     * @param slotIndex The index of the slot.
     * @return PacketResult indicating success or error.
     */
    EterBase::PacketResult<void> DeleteMallItem(uint16_t slotIndex) {
        if (slotIndex >= mallSize_) {
            return EterBase::MakeError(EterBase::PacketError::SequenceMismatch);
        }

        mallSlots_[slotIndex].reset();
        Core::EventBus::GetInstance().Publish(SafeBoxSlotUpdateEvent(true, slotIndex));
        return {};
    }

    /**
     * @brief Retrieves the item at the specified Mall slot.
     * @param slotIndex The index of the slot.
     * @return std::optional containing the item data if present.
     */
    std::optional<SafeBoxItemData> GetMallItem(uint16_t slotIndex) const {
        if (slotIndex >= mallSize_) {
            return std::nullopt;
        }
        return mallSlots_[slotIndex];
    }

    /**
     * @brief Updates the stored money in the SafeBox.
     * @param newMoney The new money amount.
     */
    void SetMoney(uint32_t newMoney) {
        money_ = newMoney;
        Core::EventBus::GetInstance().Publish(SafeBoxMoneyUpdateEvent(money_));
    }

    /**
     * @brief Gets the current money stored in the SafeBox.
     * @return The money amount.
     */
    [[nodiscard]] uint32_t GetMoney() const {
        return money_;
    }

    /**
     * @brief Gets the current capacity of the SafeBox.
     * @return The number of slots.
     */
    [[nodiscard]] uint32_t GetSafeBoxSize() const {
        return safeBoxSize_;
    }

    /**
     * @brief Gets the current capacity of the Mall.
     * @return The number of slots.
     */
    [[nodiscard]] uint32_t GetMallSize() const {
        return mallSize_;
    }

private:
    std::vector<std::optional<SafeBoxItemData>> safeBoxSlots_;
    std::vector<std::optional<SafeBoxItemData>> mallSlots_;
    
    uint32_t money_;
    uint32_t safeBoxSize_;
    uint32_t mallSize_;
    bool isUnlocked_;
};

} // namespace UserInterface::Domain
