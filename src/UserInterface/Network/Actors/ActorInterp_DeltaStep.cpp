#include "../../StdAfx.h"
#include "IActorInterpolationService.h"
#include "EterBase/ModernLogger.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "UserInterface/Core/EventBus.h"

#include <cmath>
#include <algorithm>

namespace UserInterface::Network {

    /**
     * @brief Zdarzenie aktualizacji kroku czasu dla interpolacji aktorow.
     */
    struct ActorDeltaStepEvent : public Core::IEvent {
        float deltaTime;

        explicit ActorDeltaStepEvent(float dt) : deltaTime(dt) {}
    };

    /**
     * @brief Izolowana implementacja kroku czasowego (Zero-Conflict Rule)
     */
    class ActorInterpolationService_DeltaStep final : public IActorInterpolationService {
    public:
        ActorInterpolationService_DeltaStep() = default;
        ~ActorInterpolationService_DeltaStep() override = default;

        void InterpolateHermite(EterBase::EntityId id, float startX, float startY, float endX, float endY, float factor) override {
            // Stub for this specific standalone implementation.
        }

        float SlerpRotation(float currentYaw, float targetYaw, float alpha) override {
            // Stub for this specific standalone implementation.
            return currentYaw;
        }

        void StepDelta(float deltaTime) override {
            // Zabezpieczenie przed duzymi skokami deltaTime (np. lag/zawiecha klienta)
            constexpr float MAX_DELTA_TIME = 0.1f; // Maksymalnie 100ms na klatke interpolacji
            
            float safeDeltaTime = deltaTime;
            if (safeDeltaTime > MAX_DELTA_TIME) {
                EterBase::ModernLogger::Debug("ActorInterpolationService_DeltaStep: Clamped large deltaTime from {} to {}", safeDeltaTime, MAX_DELTA_TIME);
                safeDeltaTime = MAX_DELTA_TIME;
            }

            // Opcjonalnie ignorujemy ekstremalnie male wartosci (szum)
            if (safeDeltaTime <= 0.0001f) {
                return;
            }

            // Publikacja zdarzenia przez szyne (EventBus)
            ActorDeltaStepEvent event(safeDeltaTime);
            Core::EventBus::GetInstance().Publish(event);
        }

        void Clear() override {
            // Stub for this specific standalone implementation.
        }
    };

} // namespace UserInterface::Network
