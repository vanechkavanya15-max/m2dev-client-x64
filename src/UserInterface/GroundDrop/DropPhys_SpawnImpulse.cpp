#include "../StdAfx.h"
#include "DropPhysicsSimulator.h"
#include "GroundDropEvents.h"
#include "../../EterBase/LogModern.h"

#include <cmath>
#include <random>
#include <memory>
#include <format>
#include <numbers>

namespace UserInterface::GroundDrop
{
    void DropPhysicsSimulator::SpawnImpulse(uint32_t virtualId, float originX, float originY, float originZ, float force)
    {
        EterBase::EntityId entityId{virtualId};

        // Generator liczb losowych dla kata 0-360 stopni (w radianach)
        static thread_local std::random_device rd;
        static thread_local std::mt19937 gen(rd());
        std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * std::numbers::pi_v<float>);

        float angle = angleDist(gen);

        // Obliczanie wektora predkosci (X, Y) w zaleznosci od kata i sily rzutu
        float velX = std::cos(angle) * force;
        float velY = std::sin(angle) * force;
        
        // Predkosc pionowa (Z) w celu nadania lotu parabolicznego
        float velZ = force * 1.5f;

        // Decoupling (EventBus)
        Events::DropImpulseSpawnedEvent event(entityId, velX, velY, velZ);
        UserInterface::Core::EventBus::GetInstance().Publish(event);

        // Logowanie uzywajac formatowania w standardzie C++23 (ModernLogger)
        EterBase::ModernLogger::Debug(
            "DropPhysicsSimulator::SpawnImpulse - VirtualId: {}, origin: ({:.2f}, {:.2f}, {:.2f}), vel: ({:.2f}, {:.2f}, {:.2f})",
            entityId.value(), originX, originY, originZ, velX, velY, velZ
        );
    }

    void DropPhysicsSimulator::StepPhysics(float /*deltaTime*/, float /*groundHeight*/) {}
    DropTrajectory DropPhysicsSimulator::GetTrajectory(uint32_t /*virtualId*/) const { return DropTrajectory{}; }
    void DropPhysicsSimulator::Clear() {}

    std::unique_ptr<IDropPhysicsSimulator> CreateDropPhysicsSimulator()
    {
        return std::make_unique<DropPhysicsSimulator>();
    }
}
