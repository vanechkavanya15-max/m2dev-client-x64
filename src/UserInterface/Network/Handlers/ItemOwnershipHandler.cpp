#include "../../StdAfx.h"
#include "ItemOwnershipHandler.h"
#include "../../../EterBase/LogModern.h"
#include <string_view>
#include <algorithm>

namespace Network::Handlers
{
    EterBase::PacketResult<void> ProcessItemOwnership(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketItemOwnership))
        {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "ItemOwnershipHandler: Buffer underflow");
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketItemOwnership*>(buffer.data());
        const EterBase::EntityId itemVid(packet->id);

        // Safe bounded string copy for name to avoid buffer overrun
        std::string_view nameView(packet->name, strnlen(packet->name, CHARACTER_NAME_MAX_LEN));

        ItemOwnershipEvent event(itemVid, std::string(nameView));
        UserInterface::Core::EventBus::GetInstance().Publish(event);

        return {};
    }

    bool HandleItemOwnership(std::span<const uint8_t> buffer)
    {
        return ProcessItemOwnership(buffer).has_value();
    }
}
