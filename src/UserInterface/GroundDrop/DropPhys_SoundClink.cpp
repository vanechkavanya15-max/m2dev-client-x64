#include "../StdAfx.h"
#include "IDropPhysicsSimulator.h"
#include "../Core/EventBus.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include <expected>

namespace UserInterface::GroundDrop {

    namespace {
        struct PlaySoundClinkEvent : public Core::IEvent {
            EterBase::EntityId entityId;
            EterBase::ItemVnum itemVnum;
            float posX;
            float posY;
            float posZ;

            PlaySoundClinkEvent(EterBase::EntityId ent, EterBase::ItemVnum vnum, float x, float y, float z)
                : entityId(ent), itemVnum(vnum), posX(x), posY(y), posZ(z) {}
        };
    }

    std::expected<void, EterBase::EntityError> TriggerDropSoundClink(
        EterBase::EntityId entityId,
        EterBase::ItemVnum itemVnum,
        const DropTrajectory& trajectory)
    {
        if (!trajectory.isSettled) {
            EterBase::ModernLogger::Debug("Entity {} is not settled yet, no clink sound played.", entityId.value());
            return std::unexpected(EterBase::EntityError::InvalidType);
        }

        // Publish event to EventBus for audio system (Zero-Conflict)
        Core::EventBus::GetInstance().Publish(PlaySoundClinkEvent{
            entityId,
            itemVnum,
            trajectory.posX,
            trajectory.posY,
            trajectory.posZ
        });

        EterBase::ModernLogger::Info("Triggered metallic sound clink for entity {} (vnum {}) at coordinates ({}, {}, {}).",
            entityId.value(),
            itemVnum.value(),
            trajectory.posX,
            trajectory.posY,
            trajectory.posZ);

        return {};
    }

}
