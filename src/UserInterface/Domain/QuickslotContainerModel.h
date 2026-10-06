#pragma once

#include <cstdint>
#include <array>
#include <optional>
#include <expected>
#include <variant>

#include "../Core/EventBus.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/LogModern.h"

namespace UserInterface::Domain {

/**
 * @brief Enum defining the different types of items that can be placed in a quickslot.
 */
enum class QuickslotType : uint8_t {
    None = 0,
    Item = 1,
    Skill = 2,
    Command = 3
};

/**
 * @brief Local error enum for quickslot operations.
 */
enum class QuickslotError : uint8_t {
    None = 0,
    SlotOutOfRange,
    SlotEmpty
};

/**
 * @brief Represents data for a single quickslot entry.
 */
struct QuickslotData {
    QuickslotType type; ///< The type of the quickslot item.
    
    /**
     * @brief The strongly-typed payload associated with the quickslot.
     * Uses std::variant to store the appropriate ID/slot based on type.
     */
    std::variant<std::monostate, EterBase::ItemSlot, EterBase::SkillId, uint8_t> position;
};

/**
 * @brief Event triggered when a new quickslot is added.
 */
struct QuickslotAddedEvent : public Core::IEvent {
    uint8_t slotIndex;  ///< The index of the added quickslot.
    QuickslotData data; ///< The data of the added quickslot.

    /**
     * @brief Constructs the event.
     * @param slotIndex The index of the added quickslot.
     * @param data The data of the added quickslot.
     */
    QuickslotAddedEvent(uint8_t slotIndex, QuickslotData data)
        : slotIndex(slotIndex), data(std::move(data)) {}
};

/**
 * @brief Event triggered when a quickslot is deleted.
 */
struct QuickslotDeletedEvent : public Core::IEvent {
    uint8_t slotIndex; ///< The index of the deleted quickslot.

    /**
     * @brief Constructs the event.
     * @param slotIndex The index of the deleted quickslot.
     */
    explicit QuickslotDeletedEvent(uint8_t slotIndex)
        : slotIndex(slotIndex) {}
};

/**
 * @brief Event triggered when two quickslots are swapped.
 */
struct QuickslotSwappedEvent : public Core::IEvent {
    uint8_t sourceIndex; ///< The source slot index.
    uint8_t targetIndex; ///< The target slot index.

    /**
     * @brief Constructs the event.
     * @param sourceIndex The source slot index.
     * @param targetIndex The target slot index.
     */
    QuickslotSwappedEvent(uint8_t sourceIndex, uint8_t targetIndex)
        : sourceIndex(sourceIndex), targetIndex(targetIndex) {}
};

/**
 * @brief Model managing the 36 quickslots of a character.
 * 
 * Follows modern C++23 standards, utilizing std::expected for errors and 
 * emitting events via EventBus to decouple logic from UI.
 */
class QuickslotContainerModel {
public:
    static constexpr uint8_t QUICKSLOT_MAX_NUM = 36;

    /**
     * @brief Adds or overwrites a quickslot at the specified index.
     * @param slotIndex The index (0-35) where the quickslot should be added.
     * @param data The quickslot data to add.
     * @return Success or a QuickslotError on failure.
     */
    std::expected<void, QuickslotError> AddQuickslot(uint8_t slotIndex, QuickslotData data) {
        if (slotIndex >= QUICKSLOT_MAX_NUM) {
            EterBase::ModernLogger::Error("Failed to add quickslot: slot index {} out of range", slotIndex);
            return std::unexpected(QuickslotError::SlotOutOfRange);
        }

        slots_[slotIndex] = data;
        Core::EventBus::Instance().Publish(QuickslotAddedEvent(slotIndex, data));
        
        EterBase::ModernLogger::Info("Added quickslot at {} with type {}", 
            slotIndex, static_cast<uint8_t>(data.type));
            
        return {};
    }

    /**
     * @brief Deletes the quickslot at the specified index.
     * @param slotIndex The index (0-35) of the quickslot to delete.
     * @return Success or a QuickslotError on failure.
     */
    std::expected<void, QuickslotError> DeleteQuickslot(uint8_t slotIndex) {
        if (slotIndex >= QUICKSLOT_MAX_NUM) {
            EterBase::ModernLogger::Error("Failed to delete quickslot: slot index {} out of range", slotIndex);
            return std::unexpected(QuickslotError::SlotOutOfRange);
        }

        if (!slots_[slotIndex].has_value()) {
            return std::unexpected(QuickslotError::SlotEmpty);
        }

        slots_[slotIndex].reset();
        Core::EventBus::Instance().Publish(QuickslotDeletedEvent(slotIndex));
        
        EterBase::ModernLogger::Info("Deleted quickslot at {}", slotIndex);
        
        return {};
    }

    /**
     * @brief Swaps the contents of two quickslots.
     * @param sourceIndex The index of the first quickslot.
     * @param targetIndex The index of the second quickslot.
     * @return Success or a QuickslotError on failure.
     */
    std::expected<void, QuickslotError> SwapQuickslot(uint8_t sourceIndex, uint8_t targetIndex) {
        if (sourceIndex >= QUICKSLOT_MAX_NUM || targetIndex >= QUICKSLOT_MAX_NUM) {
            EterBase::ModernLogger::Error("Failed to swap quickslots: index {} or {} out of range", 
                sourceIndex, targetIndex);
            return std::unexpected(QuickslotError::SlotOutOfRange);
        }

        if (sourceIndex == targetIndex) {
            return {};
        }

        std::swap(slots_[sourceIndex], slots_[targetIndex]);
        Core::EventBus::Instance().Publish(QuickslotSwappedEvent(sourceIndex, targetIndex));
        
        EterBase::ModernLogger::Info("Swapped quickslots {} and {}", sourceIndex, targetIndex);
        
        return {};
    }

    /**
     * @brief Gets the quickslot data at the specified index, mapping through monads.
     * @param slotIndex The index (0-35) of the quickslot.
     * @return An optional containing the quickslot data, or std::nullopt if empty/invalid.
     */
    [[nodiscard]] std::optional<QuickslotData> GetQuickslot(uint8_t slotIndex) const {
        auto indexOpt = (slotIndex < QUICKSLOT_MAX_NUM) ? std::make_optional(slotIndex) : std::nullopt;
        
        return indexOpt.and_then([this](uint8_t idx) {
            return slots_[idx];
        });
    }

private:
    std::array<std::optional<QuickslotData>, QUICKSLOT_MAX_NUM> slots_{};
};

} // namespace UserInterface::Domain
