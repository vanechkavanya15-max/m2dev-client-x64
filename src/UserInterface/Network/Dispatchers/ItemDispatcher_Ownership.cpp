#include "../../StdAfx.h"
#include "ItemDispatcher_Ownership.h"
#include "../../Packet.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"
#include "../../Core/Events/ItemEvents.h"
#include <cstring>
#include <algorithm>
#include <span>

namespace UserInterface::Network {

EterBase::PacketResult<void> DispatchItemOwnership(std::span<const uint8_t> buffer) {
    if (buffer.size() < sizeof(TPacketGCItemOwnership)) {
        EterBase::ModernLogger::Error("Buffer underflow when parsing TPacketGCItemOwnership. Expected: {}, Got: {}",
                                      sizeof(TPacketGCItemOwnership), buffer.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCItemOwnership packet{};
    std::copy_n(buffer.data(), sizeof(TPacketGCItemOwnership), reinterpret_cast<uint8_t*>(&packet));

    EterBase::EntityId vid(packet.dwVID);
    
    // Safety null termination
    packet.szName[CHARACTER_NAME_MAX_LEN] = '\0';
    std::string ownerName(packet.szName);

    auto eventResult = Core::Events::ItemOwnershipChanged::Create(vid, std::move(ownerName));
    if (!eventResult.has_value()) {
         EterBase::ModernLogger::Error("Failed to create ItemOwnershipChanged event: {}", EterBase::ToString(eventResult.error()));
         return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    EterBase::ModernLogger::Info("ItemOwnership: Item VID {} ownership granted to {}", eventResult->vid.get(), eventResult->ownerName);

    Core::EventBus::GetInstance().Publish(eventResult.value());

    return {};
}

} // namespace UserInterface::Network
