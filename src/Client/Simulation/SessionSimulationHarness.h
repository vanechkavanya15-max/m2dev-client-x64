#pragma once

#include <memory>
#include <string>
#include <vector>
#include <span>
#include "../Core/GameSession.h"
#include "../Core/WorldContext.h"
#include "../Core/INetworkPort.h"

namespace Client::Simulation {

class MockNetworkPortAdvanced : public Client::Core::INetworkPort {
public:
    [[nodiscard]] Client::Core::Result<void, Client::Core::PacketError> SendRaw(uint8_t opcode, std::span<const uint8_t> payload) override {
        return {};
    }
    [[nodiscard]] bool IsConnected() const noexcept override {
        return true;
    }
};

class VirtualPacketGenerator {
public:
    void GenerateSpawnMonsterPacket(uint32_t vid, uint32_t vnum, float x, float y, uint32_t hp) {}
    void GenerateSpawnPlayerPacket(uint32_t vid, const std::string& name, float x, float y) {}
    void GenerateDropItemPacket(uint32_t itemVid, uint32_t vnum, float x, float y) {}
};

class SessionSimulationHarness {
public:
    SessionSimulationHarness();
    ~SessionSimulationHarness();

    SessionSimulationHarness(const SessionSimulationHarness&) = delete;
    SessionSimulationHarness& operator=(const SessionSimulationHarness&) = delete;

    void AdvanceTime(float deltaSeconds);
    void StepFrames(uint32_t count);

    void SpawnMonster(uint32_t vid, uint32_t vnum, float x, float y, uint32_t hp);
    void SpawnPlayer(uint32_t vid, const std::string& name, float x, float y);
    void DropItem(uint32_t itemVid, uint32_t vnum, float x, float y);

    void AssertEntityExists(uint32_t vid) const;
    void AssertEntityDead(uint32_t vid) const;
    void AssertPlayerCoords(float expectedX, float expectedY) const;

    [[nodiscard]] Client::Core::GameSession& GetSession() noexcept { return m_session; }
    [[nodiscard]] const Client::Core::GameSession& GetSession() const noexcept { return m_session; }

private:
    Client::Core::GameSession m_session;
    std::shared_ptr<MockNetworkPortAdvanced> m_networkPort;
    std::unique_ptr<VirtualPacketGenerator> m_packetGenerator;
};

} // namespace Client::Simulation
