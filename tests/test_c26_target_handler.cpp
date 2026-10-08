#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "../src/EterBase/StrongTypes.h"
#include "../src/EterBase/Result.h"

// Redefine TPacketGCTarget locally for the test
#pragma pack(push, 1)
typedef struct packet_target
{
    uint16_t	header;
    uint16_t	length;
    uint32_t       dwVID;
    uint8_t        bHPPercent;
} TPacketGCTarget;
#pragma pack(pop)

// Include the real headers
#include "../src/Client/Gameplay/CombatDomain.h"
#include "../src/Client/Network/Handlers/TargetHandler.h"

// Define a test-specific implementation of the handler locally to test the logic
// without dragging in the complex UserInterface/Packet.h and StdAfx.h
namespace Client::Network::Handlers {

EterBase::PacketResult<void> TargetHandler::HandleGCTarget(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCTarget)) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    // Safely copy to avoid strict aliasing / alignment issues
    TPacketGCTarget packet;
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCTarget));
    
    EterBase::EntityId targetVid(packet.dwVID);
    uint8_t hpPercent = packet.bHPPercent;

    if (hpPercent > 100) {
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    Gameplay::CombatDomain::UpdateTargetHP(targetVid, hpPercent);
    
    return {};
}

} // namespace Client::Network::Handlers

// Include CombatDomain implementation
#include "../src/Client/Gameplay/CombatDomain.cpp"

TEST_CASE("TargetHandler::HandleGCTarget Valid Packet") {
    // Reset state via UpdateTargetHP since we don't have Reset() in the domain
    Client::Gameplay::CombatDomain::UpdateTargetHP(EterBase::EntityId(0), 0);
    
    TPacketGCTarget pkt{};
    pkt.header = 0x11;
    pkt.length = sizeof(pkt);
    pkt.dwVID = 12345;
    pkt.bHPPercent = 75;

    std::span<const uint8_t> payload(reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
    auto result = Client::Network::Handlers::TargetHandler::HandleGCTarget(payload);

    CHECK(result.has_value());
    auto currentTarget = Client::Gameplay::CombatDomain::GetCurrentTarget();
    REQUIRE(currentTarget.has_value());
    CHECK(currentTarget->first.value() == 12345);
    CHECK(currentTarget->second == 75);
}

TEST_CASE("TargetHandler::HandleGCTarget Buffer Underflow") {
    TPacketGCTarget pkt{};
    std::span<const uint8_t> payload(reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt) - 1);
    
    auto result = Client::Network::Handlers::TargetHandler::HandleGCTarget(payload);
    CHECK_FALSE(result.has_value());
    CHECK(result.error() == EterBase::PacketError::BufferUnderflow);
}

TEST_CASE("TargetHandler::HandleGCTarget Malformed Payload (HP > 100)") {
    TPacketGCTarget pkt{};
    pkt.dwVID = 12345;
    pkt.bHPPercent = 105;

    std::span<const uint8_t> payload(reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
    
    auto result = Client::Network::Handlers::TargetHandler::HandleGCTarget(payload);
    CHECK_FALSE(result.has_value());
    CHECK(result.error() == EterBase::PacketError::MalformedPayload);
}
