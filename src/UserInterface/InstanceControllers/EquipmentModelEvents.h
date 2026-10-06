#pragma once

#include "../Core/EventBus.h"
#include "IInstanceEquipmentModelController.h"
#include "EterBase/StrongTypes.h"

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Zdarzenie aktualizacji modelu ekwipunku postaci (do nasluchiwania przez GUI/Renderer).
     */
    struct EquipmentModelChangedEvent : public Core::IEvent
    {
        ModelPart part;
        EterBase::ItemVnum vnum;

        EquipmentModelChangedEvent(ModelPart p, EterBase::ItemVnum v)
            : part(p), vnum(v) {}
    };
}
