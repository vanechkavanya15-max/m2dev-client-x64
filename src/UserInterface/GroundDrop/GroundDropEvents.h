#pragma once

#include "../Core/EventBus.h"
#include "../../EterBase/StrongTypes.h"

namespace UserInterface::GroundDrop::Events
{
    /**
     * @brief Event published when a ground drop item is added to the batch renderer.
     */
    struct GroundDropAddedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId virtualId;
        float x;
        float y;
        float z;

        GroundDropAddedEvent(EterBase::EntityId vid, float x, float y, float z)
            : virtualId(vid), x(x), y(y), z(z) {}
    };

    /**
     * @brief Event published when a ground drop item is removed from the batch renderer.
     */
    struct GroundDropRemovedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId virtualId;

        explicit GroundDropRemovedEvent(EterBase::EntityId vid)
            : virtualId(vid) {}
    };

    /**
     * @brief Event published when an impulse is spawned for a dropping item.
     */
    struct DropImpulseSpawnedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId virtualId;
        float velX;
        float velY;
        float velZ;

        DropImpulseSpawnedEvent(EterBase::EntityId vid, float vx, float vy, float vz)
            : virtualId(vid), velX(vx), velY(vy), velZ(vz) {}
    };
}
