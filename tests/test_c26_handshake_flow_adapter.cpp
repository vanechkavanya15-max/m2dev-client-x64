#include <cassert>
#include <iostream>
#include <vector>
#include <cstring>
#include <memory>
#include <span>

#define _WIN32
namespace EterBase {
    namespace ModernLogger {
        template <typename... Args> void Error(Args&&...) {}
        template <typename... Args> void Info(Args&&...) {}
    }
}
struct CPythonCharacterManager { static CPythonCharacterManager* Instance() { return nullptr; } };

#include "../src/Client/Network/HandshakeFlowAdapter.h"
#include "../src/Client/Network/HandshakeFSM.h"
#include "../src/Client/Core/INetworkPort.h"
#include "../src/UserInterface/Packets/Packet_Handshake.h"
#include "../src/Client/Network/PhaseStateMachine.h"

class MockINetworkPort : public Client::Core::INetworkPort {
public:
    bool sendRawCalled = false;
    uint32_t receivedTime = 0;
    int32_t receivedDelta = 0;

    Client::Core::Result<void, Client::Core::PacketError> SendRaw(uint8_t opcode, std::span<const uint8_t> payload) override {
        sendRawCalled = true;
        if (payload.size() == sizeof(PacketHandshake)) {
            PacketHandshake packet;
            std::memcpy(&packet, payload.data(), sizeof(PacketHandshake));
            receivedTime = packet.time;
            receivedDelta = packet.delta;
        }
        return {};
    }
    bool IsConnected() const noexcept override { return true; }
};

class MockMultiCryptoManager : public Client::Network::MultiCryptoManager {
public:
    bool secureStateEnabled = false;
    EterBase::VoidResult<> EnableSecureState() override {
        secureStateEnabled = true;
        return {};
    }
};

class DummyOfflinePhase : public Client::Network::IPhase {
public:
    EterBase::VoidResult<> Enter() override { return {}; }
    EterBase::VoidResult<> Exit() override { return {}; }
    Client::Network::Phase GetPhase() const override { return Client::Network::Phase::Offline; }
};

class DummyHandshakePhase : public Client::Network::IPhase {
public:
    EterBase::VoidResult<> Enter() override { return {}; }
    EterBase::VoidResult<> Exit() override { return {}; }
    Client::Network::Phase GetPhase() const override { return Client::Network::Phase::Handshake; }
};

class DummyLoginPhase : public Client::Network::IPhase {
public:
    EterBase::VoidResult<> Enter() override { return {}; }
    EterBase::VoidResult<> Exit() override { return {}; }
    Client::Network::Phase GetPhase() const override { return Client::Network::Phase::Login; }
};

int main() {
    Client::Network::PhaseStateMachine phaseMachine;
    phaseMachine.RegisterPhase(std::make_unique<DummyOfflinePhase>());
    phaseMachine.RegisterPhase(std::make_unique<DummyHandshakePhase>());
    phaseMachine.RegisterPhase(std::make_unique<DummyLoginPhase>());
    
    // We must transition to Handshake before FSM complete transitions to Login
    phaseMachine.ChangePhase(Client::Network::Phase::Handshake);

    Client::Network::HandshakeFSM fsm(&phaseMachine);
    MockMultiCryptoManager cryptoManager;
    MockINetworkPort networkPort;

    Client::Network::HandshakeFlowAdapter adapter(&fsm, &cryptoManager, &networkPort);

    PacketHandshake simulatedPacket{};
    simulatedPacket.header = 0xFF;
    simulatedPacket.handshake = 12345;
    simulatedPacket.time = 50000;
    simulatedPacket.delta = 100;

    std::vector<uint8_t> payloadData(sizeof(PacketHandshake));
    std::memcpy(payloadData.data(), &simulatedPacket, sizeof(PacketHandshake));
    std::span<const uint8_t> payload(payloadData);

    uint32_t currentClientTime = 50050;

    auto result = adapter.HandleHandshakePacket(payload, currentClientTime);

    assert(result.has_value());
    assert(networkPort.sendRawCalled);
    uint32_t expectedCompensatedTime = simulatedPacket.time + simulatedPacket.delta;
    assert(networkPort.receivedTime == expectedCompensatedTime);
    assert(networkPort.receivedDelta == simulatedPacket.delta);
    assert(cryptoManager.secureStateEnabled);
    assert(fsm.GetState() == Client::Network::HandshakeState::Complete);

    std::cout << "All assertions passed!" << std::endl;
    return 0;
}
