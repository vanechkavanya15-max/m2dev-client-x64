#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "Client/Core/EventBus.h"

namespace Client::Network::Handlers {

#pragma pack(push, 1)
struct ItemGroundDelPacket
{
    uint8_t header;
    uint32_t itemVid;
};
#pragma pack(pop)
static_assert(sizeof(ItemGroundDelPacket) == 5, "ItemGroundDelPacket must be exactly 5 bytes");

struct ItemGroundDelEvent : public Client::Core::IEvent
{
    EterBase::EntityId dropVid;

    explicit ItemGroundDelEvent(EterBase::EntityId dropVid)
        : dropVid(dropVid) {}
};

class ItemGroundDelHandler
{
public:
    static EterBase::PacketResult<void> Handle(std::span<const uint8_t> buffer);
};

} // namespace Client::Network::Handlers
