#include "../../StdAfx.h"
#include "IActorInterpolationService.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "Core/EventBus.h"

#include <cmath>
#include <utility>
#include <expected>

namespace UserInterface::Network::Actors {

/**
 * @brief Event emitted when an actor's movement slides along an obstacle.
 */
struct ActorObstacleSlidedEvent final : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;
    float slideX;
    float slideY;

    ActorObstacleSlidedEvent(EterBase::EntityId id, float sx, float sy)
        : entityId(id), slideX(sx), slideY(sy) {}
};

class ActorInterpObstacleSlideHandler {
public:
    /**
     * @brief Calculates the sliding vector along an obstacle's normal and publishes the event.
     * 
     * @param interpolationService Reference to the interpolation service.
     * @param entityId The ID of the moving entity.
     * @param moveX Movement vector X component.
     * @param moveY Movement vector Y component.
     * @param normalX Wall normal X component.
     * @param normalY Wall normal Y component.
     * @return std::expected<std::pair<float, float>, EterBase::EntityError> The computed slide vector.
     */
    static std::expected<std::pair<float, float>, EterBase::EntityError> CalculateSlideVector(
        IActorInterpolationService& interpolationService,
        EterBase::EntityId entityId,
        float moveX, float moveY,
        float normalX, float normalY)
    {
        if (!entityId) {
            EterBase::ModernLogger::Error("ActorInterpObstacleSlideHandler: Invalid EntityId provided");
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        // Normalize the normal vector just in case
        float normalLength = std::sqrt(normalX * normalX + normalY * normalY);
        if (normalLength <= 0.0001f) {
            // Cannot slide on a zero normal vector, return original movement
            EterBase::ModernLogger::Warning("ActorInterpObstacleSlideHandler: Zero normal vector for entity {}", entityId.value());
            return std::pair<float, float>{moveX, moveY};
        }

        float nX = normalX / normalLength;
        float nY = normalY / normalLength;

        // V_slide = V - (V dot N) * N
        float dotProduct = moveX * nX + moveY * nY;
        
        // If the movement is actually away from the wall, no need to slide, just return movement.
        if (dotProduct > 0.0f) {
            return std::pair<float, float>{moveX, moveY};
        }

        float slideX = moveX - dotProduct * nX;
        float slideY = moveY - dotProduct * nY;

        EterBase::ModernLogger::Debug("ActorInterpObstacleSlideHandler: Calculated slide vector ({}, {}) for entity {}", slideX, slideY, entityId.value());

        ActorObstacleSlidedEvent ev(entityId, slideX, slideY);
        UserInterface::Core::EventBus::GetInstance().Publish(ev);

        return std::pair<float, float>{slideX, slideY};
    }
};

} // namespace UserInterface::Network::Actors
