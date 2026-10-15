#include <iostream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <string>

#define ETERBASE_STDAFX_H

// Mock ServerProfile.cpp to avoid compiling error
#include "../src/Client/Network/ServerProfile.h"
namespace Client::Network {
    void ServerProfile::SetHost(const std::string& host) { m_host = host; }
    void ServerProfile::SetPort(uint16_t port) { m_port = port; }
    void ServerProfile::SetProfileName(const std::string& name) { m_profileName = name; }
    [[nodiscard]] EterBase::VoidResult<> ServerProfile::SetHeaderSizeBytes(uint8_t size) {
        if (size != 1 && size != 2) return EterBase::MakeError("Header size must be 1 or 2 bytes");
        m_headerSizeBytes = size; return {};
    }
    void ServerProfile::SetFramingMode(FramingMode mode) { m_framingMode = mode; }
    void ServerProfile::SetCryptoType(CryptoType type) { m_cryptoType = type; }
    void ServerProfile::RegisterOpcode(const std::string& logicName, uint8_t opcode) { m_opcodes[logicName] = opcode; }
    void ServerProfile::SetMoveInterval(uint32_t ms) { m_moveIntervalMs = ms; }
    void ServerProfile::SetPickupInterval(uint32_t ms) { m_pickupIntervalMs = ms; }
    void ServerProfile::SetAttackCrcRequired(bool required) { m_attackCrcRequired = required; }
    [[nodiscard]] const std::string& ServerProfile::GetHost() const { return m_host; }
    [[nodiscard]] uint16_t ServerProfile::GetPort() const { return m_port; }
    [[nodiscard]] const std::string& ServerProfile::GetProfileName() const { return m_profileName; }
    [[nodiscard]] uint8_t ServerProfile::GetHeaderSizeBytes() const { return m_headerSizeBytes; }
    [[nodiscard]] FramingMode ServerProfile::GetFramingMode() const { return m_framingMode; }
    [[nodiscard]] CryptoType ServerProfile::GetCryptoType() const { return m_cryptoType; }
    [[nodiscard]] EterBase::Result<uint8_t> ServerProfile::GetOpcode(const std::string& logicName) const {
        auto it = m_opcodes.find(logicName);
        if (it == m_opcodes.end()) return EterBase::MakeError("Opcode not found");
        return it->second;
    }
    [[nodiscard]] uint32_t ServerProfile::GetMoveInterval() const { return m_moveIntervalMs; }
    [[nodiscard]] uint32_t ServerProfile::GetPickupInterval() const { return m_pickupIntervalMs; }
    [[nodiscard]] bool ServerProfile::IsAttackCrcRequired() const { return m_attackCrcRequired; }
}

#include "../src/Client/Mimic/PingPongEngine.h"
#include "../src/Client/Mimic/PingPongEngine.cpp"

void TestNoProfile() {
    Client::Mimic::PingPongEngine engine;
    std::vector<uint8_t> payload = { 0x01, 0x07, 0x00, 0x11, 0x22, 0x33, 0x44 };
    auto result = engine.ProcessPing(payload, 1000);
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::SessionClosed);
    std::cout << "[OK] TestNoProfile passed.\n";
}

void TestYmir1B() {
    Client::Network::ServerProfile profile;
    auto ignore1 = profile.SetHeaderSizeBytes(1);
    profile.SetFramingMode(Client::Network::FramingMode::Ymir1B);
    profile.RegisterOpcode("CG_PONG", 0x42);
    profile.SetMoveInterval(200);

    Client::Mimic::PingPongEngine engine;
    engine.SetProfile(&profile);
    
    assert(engine.GetHeartbeatStatus(0) == Client::Mimic::HeartbeatStatus::Disconnected);
    
    std::vector<uint8_t> pingPayload = { 0x99, 0x07, 0x00, 0xEF, 0xBE, 0xAD, 0xDE };
    auto res = engine.ProcessPing(pingPayload, 1000);
    assert(res.has_value());
    
    assert(engine.GetHeartbeatStatus(1000) == Client::Mimic::HeartbeatStatus::Healthy);
    
    auto earlyPong = engine.Update(1100);
    assert(earlyPong.empty());
    
    auto pong = engine.Update(1250);
    assert(!pong.empty());
    assert(pong.size() == 3);
    assert(pong[0] == 0x42);
    assert(pong[1] == 0x03);
    assert(pong[2] == 0x00);
    std::cout << "[OK] TestYmir1B passed.\n";
}

void TestM2Dev4B() {
    Client::Network::ServerProfile profile;
    auto ignore2 = profile.SetHeaderSizeBytes(2);
    profile.SetFramingMode(Client::Network::FramingMode::M2Dev4B);
    profile.RegisterOpcode("CG_PONG", 0x42);
    profile.SetMoveInterval(200);

    Client::Mimic::PingPongEngine engine;
    engine.SetProfile(&profile);
    
    std::vector<uint8_t> pingPayload = { 
        0x99, 0x00, 
        0x0A, 0x00, 0x00, 0x00, 
        0xEF, 0xBE, 0xAD, 0xDE 
    };
    
    auto res = engine.ProcessPing(pingPayload, 5000);
    assert(res.has_value());
    
    auto pong = engine.Update(5500);
    assert(!pong.empty());
    
    assert(pong.size() == 6);
    assert(pong[0] == 0x42);
    assert(pong[1] == 0x00);
    assert(pong[2] == 0x06);
    assert(pong[3] == 0x00);
    assert(pong[4] == 0x00);
    assert(pong[5] == 0x00);
    std::cout << "[OK] TestM2Dev4B passed.\n";
}

void TestBufferUnderflow() {
    Client::Network::ServerProfile profile;
    auto ignore3 = profile.SetHeaderSizeBytes(1);
    profile.SetFramingMode(Client::Network::FramingMode::Ymir1B);

    Client::Mimic::PingPongEngine engine;
    engine.SetProfile(&profile);

    std::vector<uint8_t> pingPayload = { 0x99, 0x07 };
    auto res = engine.ProcessPing(pingPayload, 1000);
    assert(!res.has_value());
    assert(res.error() == EterBase::PacketError::BufferUnderflow);
    std::cout << "[OK] TestBufferUnderflow passed.\n";
}

void TestHeartbeatDegradation() {
    Client::Network::ServerProfile profile;
    auto ignore4 = profile.SetHeaderSizeBytes(1);
    profile.SetFramingMode(Client::Network::FramingMode::Ymir1B);

    Client::Mimic::PingPongEngine engine;
    engine.SetProfile(&profile);

    std::vector<uint8_t> pingPayload = { 0x99, 0x07, 0x00, 0xEF, 0xBE, 0xAD, 0xDE };
    engine.ProcessPing(pingPayload, 1000);
    
    assert(engine.GetHeartbeatStatus(2000) == Client::Mimic::HeartbeatStatus::Healthy);
    assert(engine.GetHeartbeatStatus(6500) == Client::Mimic::HeartbeatStatus::Degraded);
    assert(engine.GetHeartbeatStatus(16000) == Client::Mimic::HeartbeatStatus::Disconnected);
    std::cout << "[OK] TestHeartbeatDegradation passed.\n";
}

int main() {
    std::cout << "Running PingPongEngine tests...\n";
    TestNoProfile();
    TestYmir1B();
    TestM2Dev4B();
    TestBufferUnderflow();
    TestHeartbeatDegradation();
    std::cout << "All PingPongEngine tests passed successfully.\n";
    return 0;
}
