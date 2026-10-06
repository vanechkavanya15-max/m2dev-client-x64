#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"

#include "EventBus.h"

namespace UserInterface::Core::Events
{
    /**
     * @brief Event emitted when an entity's position is updated.
     */
    struct PlayerPositionUpdated : public UserInterface::Core::IEvent
    {
        EterBase::EntityId entityId; ///< The unique identifier of the entity that moved.
        int32_t currentX;            ///< The current X coordinate of the entity.
        int32_t currentY;            ///< The current Y coordinate of the entity.
        int32_t destinationX;        ///< The destination X coordinate (if moving).
        int32_t destinationY;        ///< The destination Y coordinate (if moving).
        std::optional<uint8_t> rotation; ///< The entity's rotation (scaled 0-255, where actual degrees = rotation * 5.0f).

        PlayerPositionUpdated() = default;
        PlayerPositionUpdated(EterBase::EntityId id, int32_t curX, int32_t curY, int32_t destX, int32_t destY, std::optional<uint8_t> rot = std::nullopt)
            : entityId(id), currentX(curX), currentY(curY), destinationX(destX), destinationY(destY), rotation(rot) {}
        
        /**
         * @brief Validates the movement event data.
         * 
         * @return EterBase::PacketResult<void> Returns success if the entity ID is valid.
         */
        [[nodiscard]] EterBase::PacketResult<void> Validate() const noexcept
        {
            if (!entityId)
            {
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }
            return {};
        }
    };

    using PlayerPositionUpdatedEvent = PlayerPositionUpdated;

    /**
     * @brief Event emitted when an entity reaches its final movement destination.
     * 
     * Triggered locally by the movement engine when a scheduled path
     * finishes, or when a stop packet is received from the server.
     */
    struct DestinationReached : public UserInterface::Core::IEvent
    {
        EterBase::EntityId entityId; ///< The unique identifier of the entity that arrived.
        int32_t finalX;              ///< The final X coordinate reached by the entity.
        int32_t finalY;              ///< The final Y coordinate reached by the entity.

        DestinationReached() = default;
        DestinationReached(EterBase::EntityId id, int32_t x, int32_t y)
            : entityId(id), finalX(x), finalY(y) {}
        
        /**
         * @brief Validates the event data.
         * 
         * @return EterBase::PacketResult<void> Returns success if the entity ID is valid.
         */
        [[nodiscard]] EterBase::PacketResult<void> Validate() const noexcept
        {
            if (!entityId)
            {
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }
            return {};
        }
    };

    using DestinationReachedEvent = DestinationReached;

    /**
     * @brief Event emitted when an entity is detected as stuck.
     * 
     * Triggered if the movement engine detects that the entity hasn't
     * progressed towards its destination over a significant time period,
     * or has encountered blocking terrain unexpectedly.
     */
    struct StuckDetected
    {
        EterBase::EntityId entityId; ///< The unique identifier of the entity that is stuck.
        int32_t stuckX;              ///< The X coordinate where the entity is stuck.
        int32_t stuckY;              ///< The Y coordinate where the entity is stuck.
        
        /**
         * @brief Validates the event data.
         * 
         * @return EterBase::PacketResult<void> Returns success if the entity ID is valid.
         */
        [[nodiscard]] EterBase::PacketResult<void> Validate() const noexcept
        {
            if (!entityId)
            {
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }
            return {};
        }
    };

#pragma pack(pop)
} // namespace UserInterface::Core::Events

namespace Core::Events {
    using namespace UserInterface::Core::Events;
}

