#include "../StdAfx.h"
#include "IDropPhysicsSimulator.h"
#include "../Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::GroundDrop
{
    /**
     * @brief Domain event representing a dropped item that has settled on the ground.
     * Inherits from IEvent and defined in the cpp file to adhere to zero-conflict architecture.
     */
    struct DropSettledEvent : public Core::IEvent
    {
        EterBase::EntityId dropId;
        float finalX;
        float finalY;
        float finalZ;

        explicit DropSettledEvent(EterBase::EntityId id, float x, float y, float z)
            : dropId(id), finalX(x), finalY(y), finalZ(z)
        {
        }
    };

    namespace DropPhysSettle
    {
        /**
         * @brief Settles the physical state of a dropped item by nullifying its kinetic energy.
         * 
         * @param dropId The unique identifier of the dropped item.
         * @param trajectory The trajectory state that will be settled.
         * @return PacketResult<void> Success or SequenceMismatch if already settled.
         */
        EterBase::PacketResult<void> SettleDrop(EterBase::EntityId dropId, DropTrajectory& trajectory)
        {
            if (trajectory.isSettled)
            {
                EterBase::ModernLogger::Warning("Attempted to settle a drop (ID: {}) that is already settled.", dropId.value());
                return EterBase::MakeError(EterBase::PacketError::SequenceMismatch);
            }

            // Extinguish kinetic energy
            trajectory.velX = 0.0f;
            trajectory.velY = 0.0f;
            trajectory.velZ = 0.0f;
            
            // Set settled state
            trajectory.isSettled = true;

            EterBase::ModernLogger::Info("Drop (ID: {}) has successfully settled at pos ({}, {}, {}).", 
                                         dropId.value(), trajectory.posX, trajectory.posY, trajectory.posZ);

            // Publish the event to notify UI and other subsystems
            Core::EventBus::GetInstance().Publish(DropSettledEvent{dropId, trajectory.posX, trajectory.posY, trajectory.posZ});

            return {};
        }
    }
}
