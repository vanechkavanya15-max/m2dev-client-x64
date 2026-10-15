#include "../StdAfx.h"
#include "TransformComponentTable.h"
#include "../Packet.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

#include <cmath>
#include <vector>

namespace UserInterface::ECS {

/**
 * @brief Event triggered when an entity moves during the ECS simulation step.
 * Decouples logic from GUI updates.
 */
struct EntityMovedEvent : public Core::IEvent {
    EterBase::EntityId entityId;
    float newX;
    float newY;
    float newZ;
    float rotation;

    EntityMovedEvent(EterBase::EntityId id, float x, float y, float z, float rot)
        : entityId(id), newX(x), newY(y), newZ(z), rotation(rot) {}
};

/**
 * @brief Executes a single simulation step for movement.
 * Updates positions based on velocity and target coordinates.
 * @param table The TransformComponentTable storing SoA entity transforms.
 * @param deltaTime Time elapsed since last frame.
 * @return EterBase::PacketResult<void> Success or error.
 */
EterBase::PacketResult<void> StepSimdMovement(TransformComponentTable& table, float deltaTime) {
    if (table.Size() == 0) {
        return {};
    }

    const size_t size = table.Size();
    std::vector<bool> moved(size, false);

    // Pass 1: SIMD-friendly vectorized loop
    for (size_t i = 0; i < size; ++i) {
        float dx = table.targetX[i] - table.posX[i];
        float dy = table.targetY[i] - table.posY[i];
        
        float distSq = dx * dx + dy * dy;
        
        // Epsilon check to prevent jitter and zero-division
        if (distSq > 0.0001f) {
            float dist = std::sqrt(distSq);
            float moveDist = table.velocity[i] * deltaTime;
            
            if (moveDist >= dist) {
                table.posX[i] = table.targetX[i];
                table.posY[i] = table.targetY[i];
            } else {
                table.posX[i] += (dx / dist) * moveDist;
                table.posY[i] += (dy / dist) * moveDist;
            }
            moved[i] = true;
        }
    }

    // Pass 2: Event emission (non-vectorizable)
    auto& eventBus = Core::EventBus::GetInstance();
    for (size_t i = 0; i < size; ++i) {
        if (moved[i]) {
            EntityMovedEvent ev{
                EterBase::EntityId{table.entityIds[i]},
                table.posX[i],
                table.posY[i],
                table.posZ[i],
                table.rotation[i]
            };
            eventBus.Publish(ev);
            EterBase::ModernLogger::Debug("Entity {} moved to ({}, {})", table.entityIds[i], table.posX[i], table.posY[i]);
        }
    }

    return {};
}

} // namespace UserInterface::ECS
