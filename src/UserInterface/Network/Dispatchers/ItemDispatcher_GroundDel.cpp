#include "../../StdAfx.h"
#include "ItemDispatcher_GroundDel.h"
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace UserInterface::Network {

EterBase::PacketResult<void> DispatchItemGroundDel(std::span<const uint8_t> payload) {
    if (payload.size_bytes() < sizeof(TPacketGCItemGroundDel)) {
        EterBase::ModernLogger::Error("DispatchItemGroundDel: Buffer underflow. Expected >= {}, got {}", 
            sizeof(TPacketGCItemGroundDel), payload.size_bytes());
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCItemGroundDel*>(payload.data());
    
    EterBase::EntityId vid{packet->vid};

    EterBase::ModernLogger::Info("DispatchItemGroundDel: Removing ground item VID: {}", vid.value());

    ::Core::Events::GroundItemDeleted event{vid};
    ::UserInterface::Core::EventBus::GetInstance().Publish(event);

    return {};
}

} // namespace UserInterface::Network
