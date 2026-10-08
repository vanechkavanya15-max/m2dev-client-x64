#pragma once

#include <memory>
#include <string>
#include <vector>
#include <span>
#include "../Core/GameSession.h"
#include "../Core/WorldContext.h"
#include "MockNetworkPortAdvanced.h"

namespace Client::Simulation {

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
    [[nodiscard]] std::shared_ptr<MockNetworkPortAdvanced> GetNetworkPort() noexcept { return m_networkPort; }

private:
    Client::Core::GameSession m_session;
    std::shared_ptr<MockNetworkPortAdvanced> m_networkPort;
};

} // namespace Client::Simulation
