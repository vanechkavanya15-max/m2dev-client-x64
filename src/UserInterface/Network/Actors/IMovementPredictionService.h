#pragma once

#include <cstdint>
#include <span>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::Network
{
    struct PredictedPosition
    {
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};
        float yaw{0.0f};
        float speed{0.0f};
    };

    class IMovementPredictionService
    {
    public:
        virtual ~IMovementPredictionService() = default;

        virtual void UpdatePrediction(EterBase::EntityId id, float deltaTime) = 0;
        virtual void OnServerMovePacket(EterBase::EntityId id, float destX, float destY, float speed, uint32_t serverTime) = 0;
        virtual PredictedPosition GetPredictedPosition(EterBase::EntityId id) const = 0;
        virtual void ResetEntity(EterBase::EntityId id) = 0;
        virtual void Clear() = 0;
    };
}
