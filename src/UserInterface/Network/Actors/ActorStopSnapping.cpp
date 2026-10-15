#include "../../StdAfx.h"
#include "IMovementPredictionService.h"
#include "IActorInterpolationService.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include <expected>

namespace {
    struct ActorSnappedEvent : public UserInterface::Core::IEvent {
        uint32_t entityId;
        float finalX;
        float finalY;

        ActorSnappedEvent(uint32_t id, float x, float y)
            : entityId(id), finalX(x), finalY(y) {}
    };
}

namespace UserInterface::Network
{
    std::expected<void, EterBase::EntityError> ProcessActorStopSnapping(
        EterBase::EntityId entityId,
        float serverDestX,
        float serverDestY,
        IMovementPredictionService* predictionService,
        IActorInterpolationService* interpolationService)
    {
        if (entityId.value() == 0) {
            EterBase::ModernLogger::Error("ProcessActorStopSnapping: Invalid EntityId.");
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        if (!predictionService || !interpolationService) {
            EterBase::ModernLogger::Error("ProcessActorStopSnapping: Null service pointer for EntityId: {}.", entityId.value());
            return std::unexpected(EterBase::EntityError::InvalidType);
        }

        EterBase::ModernLogger::Debug("Processing Actor Stop Snapping for EntityId: {} at ({}, {})", entityId.value(), serverDestX, serverDestY);

        predictionService->OnServerMovePacket(entityId, serverDestX, serverDestY, 0.0f, 0);
        PredictedPosition pred = predictionService->GetPredictedPosition(entityId);

        float snappingFactor = 0.5f;
        interpolationService->InterpolateHermite(entityId, pred.x, pred.y, serverDestX, serverDestY, snappingFactor);

        ActorSnappedEvent event(entityId.value(), serverDestX, serverDestY);
        UserInterface::Core::EventBus::GetInstance().Publish(event);

        EterBase::ModernLogger::Info("Actor EntityId: {} snapped to server position ({}, {}).", entityId.value(), serverDestX, serverDestY);

        return {};
    }
}
