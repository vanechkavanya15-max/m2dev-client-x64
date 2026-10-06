#include "../StdAfx.h"
#include "IInstanceTitleRankController.h"
#include "../Core/EventBus.h"
#include "EterBase/LogModern.h"

namespace UserInterface::InstanceControllers {

    namespace {
        struct EmpireColorResolvedEvent : public Core::IEvent {
            EterBase::EntityId entityId;
            uint32_t colorArgb;

            EmpireColorResolvedEvent(EterBase::EntityId id, uint32_t color)
                : entityId(id), colorArgb(color) {}
        };
    }

    EterBase::PacketResult<void> ResolveAndPublishEmpireColor(
        EterBase::EntityId entityId, 
        const IInstanceTitleRankController& controller) 
    {
        uint8_t empireId = controller.GetEmpire();
        uint32_t colorArgb = 0;

        switch (empireId) {
            case 1: 
                colorArgb = 0xFFFF0000; // Shinsoo - Red
                EterBase::ModernLogger::Debug("Resolved empire color for entity {}: Shinsoo (Red)", entityId.value());
                break;
            case 2: 
                colorArgb = 0xFFFFFF00; // Chunjo - Yellow
                EterBase::ModernLogger::Debug("Resolved empire color for entity {}: Chunjo (Yellow)", entityId.value());
                break;
            case 3: 
                colorArgb = 0xFF0000FF; // Jinno - Blue
                EterBase::ModernLogger::Debug("Resolved empire color for entity {}: Jinno (Blue)", entityId.value());
                break;
            default:
                EterBase::ModernLogger::Warning("Unknown empire ID {} for entity {}", empireId, entityId.value());
                return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        Core::EventBus::GetInstance().Publish(EmpireColorResolvedEvent{entityId, colorArgb});
        return {};
    }

}
