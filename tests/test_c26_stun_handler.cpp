#include "src/Client/Network/Handlers/StunHandler.h"
#include "src/UserInterface/Core/EventBus.h"
#include "EterBase/LogModern.h"
#include <iostream>
#include <vector>

#pragma pack(push, 1)
typedef struct packet_stun
{
	uint16_t	header;
	uint16_t	length;
	uint32_t		vid;
} TPacketGCStun;
#pragma pack(pop)

int main() {
    TPacketGCStun packet{};
    packet.header = 0x0216;
    packet.vid = 12345;

    // Test valid payload size
    std::span<const uint8_t> validPayload(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
    auto result = Client::Network::Handlers::HandleStunPacket(validPayload);
    
    if (!result.has_value()) {
        std::cerr << "Test failed: valid payload rejected.\n";
        return 1;
    }

    // Test invalid payload size
    std::span<const uint8_t> invalidPayload(validPayload.data(), validPayload.size() - 1);
    auto result2 = Client::Network::Handlers::HandleStunPacket(invalidPayload);
    if (result2.has_value() || result2.error() != EterBase::PacketError::BufferUnderflow) {
        std::cerr << "Test failed: invalid payload not correctly rejected.\n";
        return 1;
    }

    std::cout << "All tests passed.\n";
    return 0;
}
