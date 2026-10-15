#pragma once

#include <span>
#include <cstdint>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

namespace Core::Events {
    struct GroundItemDeleted : public UserInterface::Core::IEvent {
        EterBase::EntityId vid;

        explicit GroundItemDeleted(EterBase::EntityId vid) : vid(vid) {}
    };
} // namespace Core::Events

namespace UserInterface::Network {
    /**
     * @brief Dispatches the HEADER_GC_ITEM_GROUND_DEL network packet.
     * @param payload The binary span of the packet payload.
     * @return EterBase::PacketResult<void> Returns empty success, or PacketError on underflow.
     */
    EterBase::PacketResult<void> DispatchItemGroundDel(std::span<const uint8_t> payload);
} // namespace UserInterface::Network
