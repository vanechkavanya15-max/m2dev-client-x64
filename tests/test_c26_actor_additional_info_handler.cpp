#include <iostream>
#include <vector>
#include <cstdint>
#include <cassert>
#include "Client/Network/Handlers/ActorAdditionalInfoHandler.h"
#include "Client/Network/PendingSpawnRegistry.h"
#include "Client/Network/ActorPacketCodec.h"
#include "EterBase/Result.h"

int main() {
    Client::Network::PendingSpawnRegistry registry;
    
    // Create an entity to test against
    EterBase::EntityId vid(12345);
    Client::Network::TPacketGCCharacterAdd spawnPacket{};
    spawnPacket.dwVID = vid.value();
    
    auto spawnResult = registry.RegisterSpawn(vid, spawnPacket);
    assert(spawnResult.has_value() && "Spawn registration failed");

    // Test 1: Valid TPacketGCCharacterAdditionalInfo
    Client::Network::TPacketGCCharacterAdditionalInfo validPacket{};
    validPacket.dwVID = vid.value();
    validPacket.dwLevel = 50;
    
    std::vector<uint8_t> validPayload(reinterpret_cast<uint8_t*>(&validPacket), reinterpret_cast<uint8_t*>(&validPacket) + sizeof(validPacket));
    std::span<const uint8_t> validSpan{validPayload};
    
    auto resultValid = Client::Network::Handlers::HandleActorAdditionalInfo(validSpan, registry);
    assert(resultValid.has_value() && "HandleActorAdditionalInfo should succeed with valid payload");

    // Verify it was correctly updated in registry
    auto takeResult = registry.TakeSpawn(vid);
    assert(takeResult.has_value() && "Should be able to take spawn after additional info");
    assert(takeResult->additionalInfo.has_value() && "Additional info should be present");
    assert(takeResult->additionalInfo->dwLevel == 50 && "Data should match");

    // Test 2: Malformed Payload
    std::vector<uint8_t> malformedPayload(sizeof(Client::Network::TPacketGCCharacterAdditionalInfo) - 1, 0);
    std::span<const uint8_t> malformedSpan{malformedPayload};

    auto resultMalformed = Client::Network::Handlers::HandleActorAdditionalInfo(malformedSpan, registry);
    assert(!resultMalformed.has_value() && "HandleActorAdditionalInfo should fail with malformed payload");
    assert(resultMalformed.error() == EterBase::PacketError::MalformedPayload && "Error should be MalformedPayload");

    std::cout << "All ActorAdditionalInfoHandler tests passed successfully!" << std::endl;
    return 0;
}
