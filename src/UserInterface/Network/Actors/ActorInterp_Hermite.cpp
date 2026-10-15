#include "../../StdAfx.h"
#include "IActorInterpolationService.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "../../Core/EventBus.h"

#include <cmath>
#include <numbers>
#include <memory>
#include <algorithm>

namespace UserInterface::Network {

namespace {

    /**
     * @brief Zdarzenie aktualizacji pozycji aktora podczas interpolacji.
     * 
     * Implementacja zgodnie z zasada Zero-Conflict (zdarzenie lokalne).
     */
    struct ActorPositionUpdateEvent : public Core::IEvent {
        EterBase::EntityId entityId;
        float x;
        float y;

        ActorPositionUpdateEvent(EterBase::EntityId id, float newX, float newY)
            : entityId(id), x(newX), y(newY) {}
    };

    /**
     * @brief Implementacja serwisu interpolacji Hermite'a dla aktorow.
     */
    class ActorHermiteInterpolationService final : public IActorInterpolationService {
    public:
        ActorHermiteInterpolationService() = default;
        ~ActorHermiteInterpolationService() override = default;

        void InterpolateHermite(EterBase::EntityId id, float startX, float startY, float endX, float endY, float factor) override {
            // Bezpieczne ograniczenie faktora (0.0 do 1.0)
            float t = std::clamp(factor, 0.0f, 1.0f);

            // Obliczenie funkcji bazowych (Cubic Hermite)
            float t2 = t * t;
            float t3 = t2 * t;

            // Proxy dla Hermite'a (Smoothstep) uzywane w grach przy braku wektorow stycznych (zerowe pochodne)
            // h00(t) = 2t^3 - 3t^2 + 1
            // h01(t) = -2t^3 + 3t^2
            float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
            float h01 = -2.0f * t3 + 3.0f * t2;

            float interpolatedX = h00 * startX + h01 * endX;
            float interpolatedY = h00 * startY + h01 * endY;

            // Publikacja zdarzenia o nowej pozycji
            ActorPositionUpdateEvent event(id, interpolatedX, interpolatedY);
            Core::EventBus::GetInstance().Publish(event);

            EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "Actor {} interpolated to X: {}, Y: {}", id.value(), interpolatedX, interpolatedY);
        }

        float SlerpRotation(float currentYaw, float targetYaw, float alpha) override {
            float a = std::clamp(alpha, 0.0f, 1.0f);
            
            // Obliczenie najkrotszej sciezki na okregu (katy w stopniach)
            float diff = std::fmod(targetYaw - currentYaw, 360.0f);
            if (diff > 180.0f) {
                diff -= 360.0f;
            }
            if (diff < -180.0f) {
                diff += 360.0f;
            }

            float interpolatedYaw = currentYaw + diff * a;
            interpolatedYaw = std::fmod(interpolatedYaw, 360.0f);
            if (interpolatedYaw < 0.0f) {
                interpolatedYaw += 360.0f;
            }

            EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "SlerpRotation yaw from {} to {}, alpha {}, result {}", currentYaw, targetYaw, a, interpolatedYaw);
            return interpolatedYaw;
        }

        void StepDelta(float deltaTime) override {
            // Miejsce na ewentualna inkrementacje globalnego faktora czasu
            EterBase::ModernLogger::Log(EterBase::LogLevel::Trace, "Interpolation StepDelta: {}s", deltaTime);
        }

        void Clear() override {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Info, "Actor interpolation state cleared.");
        }
    };

} // anonymous namespace

// Opcjonalna fabryka eksportujaca konkretna instancje serwisu bez modyfikacji glownego naglowka.
std::unique_ptr<IActorInterpolationService> CreateActorHermiteInterpolationService() {
    return std::make_unique<ActorHermiteInterpolationService>();
}

} // namespace UserInterface::Network
