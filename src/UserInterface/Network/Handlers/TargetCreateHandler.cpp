#include "../../StdAfx.h"
#include "TargetCreateHandler.h"
#include "../../../EterBase/LogModern.h"
#include <cstring>
#include <algorithm>

namespace Network::Handlers
{
    EterBase::PacketResult<void> ProcessTargetCreate(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketTargetCreateNew))
        {
            EterBase::ModernLogger::Error("TargetCreateHandler: Buffer underflow (size: {})", buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketTargetCreateNew*>(buffer.data());

        // We use std::string_view for safe string handling
        std::string targetName(packet->targetName, std::min(sizeof(packet->targetName), strnlen(packet->targetName, sizeof(packet->targetName))));

        EterBase::EntityId vid(packet->vid);
        TargetCreateType type = static_cast<TargetCreateType>(packet->type);

        EterBase::ModernLogger::Debug("TargetCreateHandler: target created [id: {}, name: {}, vid: {}, type: {}]", 
            packet->id, targetName, packet->vid, static_cast<uint8_t>(packet->type));

        // Update the C++ memory state / Emit event
        UserInterface::Core::EventBus::GetInstance().Publish(
            TargetCreateEvent(packet->id, targetName, vid, type)
        );

        return {};
    }

    bool HandleTargetCreate(std::span<const uint8_t> buffer)
    {
        return ProcessTargetCreate(buffer).has_value();
    }
}
