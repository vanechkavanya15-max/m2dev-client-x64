#include "../src/Client/Core/GameSession.h"
#include <iostream>
#include <cassert>

using namespace Client::Core;

class MockPort : public INetworkPort {
public:
    bool connected = true;
    uint8_t lastOpcode = 0;

    Result<void, PacketError> SendRaw(uint8_t opcode, std::span<const uint8_t> /*payload*/) override {
        if (!connected) return std::unexpected(PacketError::Timeout);
        lastOpcode = opcode;
        return {};
    }

    bool IsConnected() const noexcept override {
        return connected;
    }
};

void TestGameSessionBasics() {
    auto port = std::make_shared<MockPort>();
    GameSession session(port);

    // Initial state
    assert(session.GetWorldContext().localPlayerVid.get() == 0);
    assert(!session.GetWorldContext().isDead);

    // Attack command valid
    AttackCommand atkCmd{.targetVid = EntityVid(1234), .attackType = 0};
    auto atkRes = session.Execute(atkCmd);
    assert(atkRes.has_value());

    // Attack invalid target
    AttackCommand invalidAtk{.targetVid = EntityVid(0), .attackType = 0};
    auto invalidRes = session.Execute(invalidAtk);
    assert(!invalidRes.has_value());
    assert(invalidRes.error() == CommandError::InvalidTarget);

    // Move command
    MoveCommand moveCmd{.destination = MapCoords{100.0f, 200.0f, 0.0f}, .rotation = 45.0f, .moveType = 1};
    auto moveRes = session.Execute(moveCmd);
    assert(moveRes.has_value());
    assert(session.GetWorldContext().localPlayerCoords.x == 100.0f);
    assert(session.GetWorldContext().localPlayerCoords.y == 200.0f);

    // Disconnected state
    port->connected = false;
    auto discRes = session.Execute(atkCmd);
    assert(!discRes.has_value());
    assert(discRes.error() == CommandError::Disconnected);
}

int main() {
    std::cout << "Running GameSession tests...\n";
    TestGameSessionBasics();
    std::cout << "All GameSession tests passed successfully.\n";
    return 0;
}
