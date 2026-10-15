#include "../StdAfx.h"
#include "TransformComponentTable.h"
#include "../Packet.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "EterBase/StrongTypes.h"
#include "../Core/EventBus.h"
#include <cmath>
#include <expected>

namespace UserInterface::ECS
{
    /**
     * @brief Computes facing angles for all entities in the transform table using auto-vectorizable loops.
     * 
     * Calculates the required facing rotation angle to look at their target position, and updates the rotation field.
     * After calculating in an auto-vectorizable loop, it notifies the UI of the state change via the EventBus in a separate loop.
     * 
     * @param table Reference to the TransformComponentTable SoA.
     * @return EterBase::PacketResult<void> Success or an error code.
     */
    EterBase::PacketResult<void> UpdateFacingAngles(TransformComponentTable& table)
    {
        const size_t count = table.Size();
        if (count == 0)
        {
            return {};
        }

        // Loop 1: Data computation (Vectorizable by the compiler)
        // We avoid calling EventBus or any complex functions inside this loop to ensure
        // the compiler can vectorize it properly.
        for (size_t i = 0; i < count; ++i)
        {
            float posX = table.posX[i];
            float posY = table.posY[i];
            float targetX = table.targetX[i];
            float targetY = table.targetY[i];

            float dirX = targetX - posX;
            float dirY = targetY - posY;

            if (std::abs(dirX) > 0.001f || std::abs(dirY) > 0.001f)
            {
                table.rotation[i] = std::atan2(dirY, dirX) * 180.0f / 3.14159265f;
            }
        }

        // Loop 2: Side effects (Scalar)
        // We notify the EventBus for all entities. Ideally we'd only notify ones that changed,
        // but for now, we publish an event for each entity that has a target different from its position.
        for (size_t i = 0; i < count; ++i)
        {
            float dirX = table.targetX[i] - table.posX[i];
            float dirY = table.targetY[i] - table.posY[i];
            
            if (std::abs(dirX) > 0.001f || std::abs(dirY) > 0.001f)
            {
                UserInterface::Core::EventBus::GetInstance().Publish(
                    UserInterface::Core::TargetBoardRefreshEvent(table.entityIds[i])
                );
            }
        }

        EterBase::ModernLogger::Info("Updated facing angles for {} entities in batch.", count);
        return {};
    }
}
