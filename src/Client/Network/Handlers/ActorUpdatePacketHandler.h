#pragma once

#include <span>
#include <cstdint>
#include <array>
#include "EterBase/PacketResult.h"
#include "EterBase/StrongTypes.h"
#include "../Protocol/Protocol.h"

namespace Client::Network::Handlers {

struct ActorVisualUpdateData {
    EterBase::EntityId vid;
    std::array<uint16_t, CHR_EQUIPPART_NUM> parts{};
    uint8_t movingSpeed{0};
    uint8_t attackSpeed{0};
    uint8_t stateFlag{0};
    std::array<uint32_t, 2> affectFlags{};
    uint32_t guildId{0};
    int16_t alignment{0};
    uint8_t pkMode{0};
    uint32_t mountVnum{0};
};

// Klasa obslugujaca pakiety zmiany ekwipunku i wygladu aktora
class ActorUpdatePacketHandler {
public:
    ActorUpdatePacketHandler() = delete;
    ~ActorUpdatePacketHandler() = delete;

    [[nodiscard]] static EterBase::PacketResult<ActorVisualUpdateData> HandleCharacterUpdate(std::span<const uint8_t> payload) noexcept;
    [[nodiscard]] static EterBase::PacketResult<ActorVisualUpdateData> HandleCharacterUpdate2(std::span<const uint8_t> payload) noexcept;
};

} // namespace Client::Network::Handlers
