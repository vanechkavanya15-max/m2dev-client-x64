#pragma once

#include <cstdint>
#include <string>
#include "../EventBus.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/Result.h"

namespace UserInterface::Core::Events {

/**
 * @brief Event triggered when ownership of an item changes on the ground.
 */
struct ItemOwnershipChanged : public IEvent {
    EterBase::EntityId vid; ///< The entity ID of the item.
    std::string ownerName;  ///< The name of the new owner.

    /**
     * @brief Factory method to create an ItemOwnershipChanged event.
     * @param vid The entity ID of the item.
     * @param ownerName The name of the new owner.
     * @return std::expected<ItemOwnershipChanged, EterBase::EntityError> The created event or an error.
     */
    static std::expected<ItemOwnershipChanged, EterBase::EntityError> Create(EterBase::EntityId vid, std::string ownerName) {
        if (!vid) {
             return std::unexpected(EterBase::EntityError::NotFound);
        }
        return ItemOwnershipChanged(vid, std::move(ownerName));
    }

private:
    /**
     * @brief Private constructor.
     * @param vid The entity ID of the item.
     * @param ownerName The name of the new owner.
     */
    ItemOwnershipChanged(EterBase::EntityId vid, std::string ownerName)
        : vid(vid), ownerName(std::move(ownerName)) {}
};

} // namespace UserInterface::Core::Events
