#include "StdAfx.h"
#include "TargetDeleteHandler.h"
#include "../../Core/EventBus.h"
#include "../../Core/Events.h"

namespace Network::Handlers
{
    EterBase::PacketResult<void> ProcessTargetDelete(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketTargetDelete))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketTargetDelete*>(buffer.data());
        const EterBase::EntityId targetId(packet->targetId);

        // Opublikuj zdarzenie do usuniecia celu, aby odpiac sie od UI
        Core::EventBus::GetInstance().Publish(Core::Events::TargetDelete{targetId.value()});

        return {};
    }

    bool HandleTargetDelete(std::span<const uint8_t> buffer)
    {
        return ProcessTargetDelete(buffer).has_value();
    }
}
