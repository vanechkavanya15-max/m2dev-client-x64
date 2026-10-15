#include "../../StdAfx.h"
#include <cstdint>
#include <optional>
#include <cmath>

#include "IMovementPredictionService.h"
#include "../../Core/EventBus.h"
#include "../../Core/MovementEvents.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"

namespace UserInterface::Network
{
    /**
     * @brief Retrieves the predicted position for an actor and publishes it as a sync event to the EventBus.
     * 
     * @param predictionService The movement prediction service containing the updated positions.
     * @param id The entity ID of the actor to synchronize.
     * @return EterBase::PacketResult<void> Success or error context.
     */
    EterBase::PacketResult<void> PublishPredictedPosition(const IMovementPredictionService& predictionService, EterBase::EntityId id)
    {
        if (!id)
        {
            EterBase::ModernLogger::Error("PublishPredictedPosition: Invalid EntityId provided.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        PredictedPosition pos = predictionService.GetPredictedPosition(id);

        int32_t x = static_cast<int32_t>(pos.x);
        int32_t y = static_cast<int32_t>(pos.y);

        // Map rotation to 0-72 segments representing 0-360 degrees.
        float normalizedYaw = pos.yaw;
        if (normalizedYaw < 0.0f) {
            normalizedYaw = 360.0f + std::fmod(normalizedYaw, 360.0f);
        } else if (normalizedYaw >= 360.0f) {
            normalizedYaw = std::fmod(normalizedYaw, 360.0f);
        }
        
        std::optional<uint8_t> rotationSegments = static_cast<uint8_t>(normalizedYaw / 5.0f);

        Core::Events::PlayerPositionUpdated event(id, x, y, x, y, rotationSegments);

        if (auto result = event.Validate(); !result.has_value())
        {
            EterBase::ModernLogger::Warning("PublishPredictedPosition: Invalid position event data for EntityId {}", id.value());
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        EterBase::ModernLogger::Debug("PublishPredictedPosition: Broadcasting predicted sync pos for EntityId {}: ({}, {}, yaw: {})", id.value(), x, y, pos.yaw);
        Core::EventBus::GetInstance().Publish(event);

        return {};
    }
}
