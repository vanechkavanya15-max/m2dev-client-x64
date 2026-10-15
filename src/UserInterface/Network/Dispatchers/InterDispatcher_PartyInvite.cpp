#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

#include <expected>
#include <span>
#include <string_view>
#include <cstring>
#include <string>

namespace UserInterface::Network::Dispatchers {

/**
 * @brief Zero-Conflict C++23 Event published when a party invite is received.
 */
struct PartyInviteEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId leaderId;

    explicit PartyInviteEvent(EterBase::EntityId leader) : leaderId(leader) {}
};

/**
 * @brief Event published when a party member is added.
 */
struct PartyAddEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId pid;
    std::string name;

    PartyAddEvent(EterBase::EntityId p_pid, std::string_view p_name) 
        : pid(p_pid), name(p_name) {}
};

/**
 * @brief Parses and dispatches the TPacketGCPartyInvite packet.
 *
 * @param packetData Binary payload of the packet.
 * @return EterBase::PacketResult<void> Success or PacketError on failure.
 */
EterBase::PacketResult<void> DispatchPartyInvite(std::span<const uint8_t> packetData) {
    if (packetData.size() < sizeof(TPacketGCPartyInvite)) {
        EterBase::ModernLogger::Error("DispatchPartyInvite: Buffer underflow. Expected >= {} bytes, got {}", 
            sizeof(TPacketGCPartyInvite), packetData.size());
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCPartyInvite*>(packetData.data());

    if (packet->length < sizeof(TPacketGCPartyInvite)) {
        EterBase::ModernLogger::Error("DispatchPartyInvite: Malformed payload length.");
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    EterBase::EntityId leaderId{packet->leader_pid};

    EterBase::ModernLogger::Info("DispatchPartyInvite: Received party invite from Leader PID: {}", leaderId.value());

    UserInterface::Core::EventBus::GetInstance().Publish(PartyInviteEvent{leaderId});

    return {};
}

/**
 * @brief Parses and dispatches the TPacketGCPartyAdd packet.
 *
 * @param packetData Binary payload of the packet.
 * @return EterBase::PacketResult<void> Success or PacketError on failure.
 */
EterBase::PacketResult<void> DispatchPartyAdd(std::span<const uint8_t> packetData) {
    if (packetData.size() < sizeof(TPacketGCPartyAdd)) {
        EterBase::ModernLogger::Error("DispatchPartyAdd: Buffer underflow. Expected >= {} bytes, got {}", 
            sizeof(TPacketGCPartyAdd), packetData.size());
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCPartyAdd*>(packetData.data());

    if (packet->length < sizeof(TPacketGCPartyAdd)) {
        EterBase::ModernLogger::Error("DispatchPartyAdd: Malformed payload length.");
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    EterBase::EntityId pid{packet->pid};
    
    // Safely read fixed-size char array
    size_t nameLen = strnlen(packet->name, CHARACTER_NAME_MAX_LEN);
    std::string_view nameView(packet->name, nameLen);

    EterBase::ModernLogger::Info("DispatchPartyAdd: Added party member PID: {}, Name: {}", pid.value(), nameView);

    UserInterface::Core::EventBus::GetInstance().Publish(PartyAddEvent{pid, nameView});

    return {};
}

} // namespace UserInterface::Network::Dispatchers
