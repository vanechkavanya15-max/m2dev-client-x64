#include "SessionSimulationHarness.h"
#include <stdexcept>
#include <cmath>
#include <span>

namespace Client::Simulation {

SessionSimulationHarness::SessionSimulationHarness()
    : m_networkPort(std::make_shared<MockNetworkPortAdvanced>()),
      m_packetGenerator(std::make_unique<VirtualPacketGenerator>())
{
    m_session.SetNetworkPort(m_networkPort);
}

SessionSimulationHarness::~SessionSimulationHarness() = default;

void SessionSimulationHarness::AdvanceTime(float deltaSeconds) {
    m_session.Tick(deltaSeconds);
}

void SessionSimulationHarness::StepFrames(uint32_t count) {
    float fixedDelta = 1.0f / 60.0f;
    for (uint32_t i = 0; i < count; ++i) {
        AdvanceTime(fixedDelta);
    }
}

void SessionSimulationHarness::SpawnMonster(uint32_t vid, uint32_t vnum, float x, float y, uint32_t hp) {
    m_packetGenerator->GenerateSpawnMonsterPacket(vid, vnum, x, y, hp);
}

void SessionSimulationHarness::SpawnPlayer(uint32_t vid, const std::string& name, float x, float y) {
    m_packetGenerator->GenerateSpawnPlayerPacket(vid, name, x, y);
}

void SessionSimulationHarness::DropItem(uint32_t itemVid, uint32_t vnum, float x, float y) {
    m_packetGenerator->GenerateDropItemPacket(itemVid, vnum, x, y);
}

void SessionSimulationHarness::AssertEntityExists(uint32_t /*vid*/) const {
    // Mock assertion
}

void SessionSimulationHarness::AssertEntityDead(uint32_t /*vid*/) const {
    // Mock assertion
}

void SessionSimulationHarness::AssertPlayerCoords(float expectedX, float expectedY) const {
    const auto& coords = m_session.GetWorldContext().localPlayerCoords;
    if (std::abs(coords.x - expectedX) > 0.1f || std::abs(coords.y - expectedY) > 0.1f) {
        throw std::runtime_error("Player coordinates do not match expected values.");
    }
}

} // namespace Client::Simulation
