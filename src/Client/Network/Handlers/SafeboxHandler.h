#pragma once

#include <span>
#include <cstdint>
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "Client/Gameplay/TradeDomain.h"
#include "UserInterface/Core/EventBus.h"

namespace Client::Network::Handlers {

struct SafeBoxSizeChangedEvent : public UserInterface::Core::IEvent {
    uint8_t size;
    explicit SafeBoxSizeChangedEvent(uint8_t size) : size(size) {}
};

struct SafeBoxWrongPasswordEvent : public UserInterface::Core::IEvent {
    SafeBoxWrongPasswordEvent() = default;
};

EterBase::PacketResult<void> HandleSafeboxSet(std::span<const uint8_t> buffer, Client::Gameplay::SafeBox& safeBox);
EterBase::PacketResult<void> HandleSafeboxDel(std::span<const uint8_t> buffer, Client::Gameplay::SafeBox& safeBox);
EterBase::PacketResult<void> HandleSafeboxSize(std::span<const uint8_t> buffer);
EterBase::PacketResult<void> HandleSafeboxWrongPassword(std::span<const uint8_t> buffer);

} // namespace Client::Network::Handlers
