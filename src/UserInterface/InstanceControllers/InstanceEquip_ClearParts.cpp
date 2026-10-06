#include "../StdAfx.h"
#include "IInstanceEquipmentModelController.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

namespace UserInterface::InstanceControllers
{
    namespace
    {
        /**
         * @brief Event published when all equipment parts of an instance are cleared.
         */
        struct EquipmentPartsClearedEvent : public Core::IEvent
        {
            EquipmentPartsClearedEvent() = default;
        };
    }

    void IInstanceEquipmentModelController::ClearAllParts()
    {
        EterBase::ModernLogger::Info("Clearing all equipment parts for instance despawn.");

        for (uint8_t i = 0; i < static_cast<uint8_t>(ModelPart::MaxParts); ++i)
        {
            auto result = ClearPart(static_cast<ModelPart>(i));
            if (!result.has_value())
            {
                EterBase::ModernLogger::Error("Failed to clear equipment part {}: {}", i, result.error());
            }
        }

        // Notify GUI and other subsystems
        Core::EventBus::GetInstance().Publish(EquipmentPartsClearedEvent{});
    }
}
