#pragma once

#include <memory>
#include <string_view>
#include <cstdint>
#include "Client/Core/GameSession.h"
#include "NetworkStreamPort.h"

class CNetworkStream;

namespace Client::Bridge {

class StranglerFacade {
public:
    static StranglerFacade& Instance() noexcept;

    void Initialize(CNetworkStream* networkStream);
    void Shutdown();

    [[nodiscard]] std::shared_ptr<Client::Core::GameSession> GetSession() const noexcept { return m_session; }
    [[nodiscard]] Client::Core::WorldContext& GetWorldContext() noexcept;

    // ========================================================================
    // Metody delegujące (In-place Strangler Pattern dla UserInterface)
    // ========================================================================
    [[nodiscard]] bool ExecuteAttack(uint32_t victimVid, uint8_t attackType);
    [[nodiscard]] bool ExecuteMove(float x, float y, float z, float rot, uint8_t moveType);
    [[nodiscard]] bool ExecuteUseSkill(uint32_t skillId, uint32_t targetVid);
    [[nodiscard]] bool ExecuteUseItem(uint16_t slot);
    [[nodiscard]] bool ExecuteDropItem(uint16_t slot, uint32_t count);
    [[nodiscard]] bool ExecutePickupItem(uint32_t itemVid);
    [[nodiscard]] bool ExecuteChat(std::string_view msg, uint8_t type);
    [[nodiscard]] bool ExecuteWhisper(std::string_view target, std::string_view msg);

private:
    StranglerFacade() = default;
    ~StranglerFacade() = default;

    std::shared_ptr<NetworkStreamPort> m_port;
    std::shared_ptr<Client::Core::GameSession> m_session;
};

} // namespace Client::Bridge
