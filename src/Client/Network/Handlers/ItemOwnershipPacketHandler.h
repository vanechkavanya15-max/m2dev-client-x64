#pragma once

#include <span>
#include <string>
#include <cstdint>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "Client/Core/EventBus.h"
#include "../Protocol/ProtocolTypes.h"

namespace Client::Network::Handlers
{
#pragma pack(push, 1)
    struct PacketItemOwnership
    {
        uint16_t header;
        uint16_t length;
        uint32_t dwVID;
        char     szName[Client::Network::Protocol::CHARACTER_NAME_MAX_LEN + 1];
    };
#pragma pack(pop)

    struct ItemOwnershipEvent : public Client::Core::IEvent
    {
        EterBase::EntityId vid;
        std::string ownerName;

        ItemOwnershipEvent(EterBase::EntityId v, std::string name)
            : vid(v), ownerName(std::move(name)) {}
    };

    EterBase::PacketResult<void> ProcessItemOwnership(std::span<const uint8_t> buffer);
}
