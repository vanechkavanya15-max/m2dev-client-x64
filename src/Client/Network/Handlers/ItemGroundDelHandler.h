#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../../UserInterface/Core/EventBus.h"

#pragma pack(push, 1)
struct ItemGroundDelPacket
{
    uint8_t header;
    uint32_t itemVid;
};
#pragma pack(pop)
static_assert(sizeof(ItemGroundDelPacket) == 5, "ItemGroundDelPacket must be exactly 5 bytes");

struct ItemGroundDelEvent : public UserInterface::Core::IEvent
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
