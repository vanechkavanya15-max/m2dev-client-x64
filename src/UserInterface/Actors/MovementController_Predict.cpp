#include "../StdAfx.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include <cmath>

namespace UserInterface::Actors
{
    struct DeadReckoningPredictedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId entityId;
        int32_t predictedX;
        int32_t predictedY;
        DeadReckoningPredictedEvent(EterBase::EntityId id, int32_t x, int32_t y)
            : entityId(id), predictedX(x), predictedY(y) {}
    };

    class MovementControllerPredict
    {
    public:
        static void PredictPosition(EterBase::EntityId entityId, int32_t currentX, int32_t currentY, float velocityX, float velocityY, float deltaSeconds)
        {
            int32_t predX = currentX + static_cast<int32_t>(std::round(velocityX * deltaSeconds));
            int32_t predY = currentY + static_cast<int32_t>(std::round(velocityY * deltaSeconds));

            UserInterface::Core::EventBus::GetInstance().Publish(DeadReckoningPredictedEvent(entityId, predX, predY));
            EterBase::ModernLogger::Debug("MovementControllerPredict: Entity {} predicted ({}, {})", entityId.get(), predX, predY);
        }
    };
}
