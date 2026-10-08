#pragma once

#include <span>
#include <cstdint>
#include <EterBase/PacketResult.h>
#include <EterBase/StrongTypes.h>

namespace Client::Network::Handlers {

struct ActorVisualUpdateData {
    EterBase::EntityId vid{0};
    uint16_t armorVnum{0};
    uint16_t weaponVnum{0};
    uint16_t hairVnum{0};
    uint8_t movingSpeed{0};
    uint8_t attackSpeed{0};
    uint8_t stateFlag{0};
    uint32_t affectFlags[2]{0, 0};
    uint32_t guildId{0};
    int16_t alignment{0};
    uint8_t pkMode{0};
    uint32_t mountVnum{0};
};

class ActorUpdateHandler {
public:
    ActorUpdateHandler() = default;
    ~ActorUpdateHandler() = default;

    [[nodiscard]] EterBase::PacketResult<ActorVisualUpdateData> HandleActorUpdate(std::span<const uint8_t> payload);
};

} // namespace Client::Network::Handlers
