#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/PacketResult.h"
#include "Client/Core/EventBus.h"

namespace Client::Network::Handlers {

struct CharacterCreateFailureEvent : public Client::Core::IEvent {
    uint8_t type;
    explicit CharacterCreateFailureEvent(uint8_t t) : type(t) {}
};

struct CharacterDeleteFailureEvent : public Client::Core::IEvent {
    explicit CharacterDeleteFailureEvent() = default;
};

class CharacterCreateDeleteHandler {
public:
    static EterBase::PacketResult<void> HandleCreateFailure(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleDeleteFailure(std::span<const uint8_t> payload);
};

} // namespace Client::Network::Handlers
