#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include "../../EterBase/Result.h"
#include "../Network/ServerProfile.h"

namespace Client::Mimic {

enum class HeartbeatStatus : uint8_t {
    Healthy = 0,
    Degraded,
    Disconnected
};

class PingPongEngine {
public:
    PingPongEngine() = default;
    ~PingPongEngine() = default;

    PingPongEngine(const PingPongEngine&) = delete;
    PingPongEngine& operator=(const PingPongEngine&) = delete;
    PingPongEngine(PingPongEngine&&) = default;
    PingPongEngine& operator=(PingPongEngine&&) = default;

    void SetProfile(const Network::ServerProfile* profile);

    EterBase::PacketResult<void> ProcessPing(std::span<const uint8_t> payload, uint32_t currentClientTime);
    
    std::vector<uint8_t> Update(uint32_t currentClientTime);

    [[nodiscard]] HeartbeatStatus GetHeartbeatStatus(uint32_t currentClientTime) const;

    void SetSimulatedReactionTime(uint32_t ms) { m_reactionDelayMs = ms; }
    [[nodiscard]] uint32_t GetReactionTime() const { return m_reactionDelayMs; }

private:
    const Network::ServerProfile* m_profile{nullptr};

    uint32_t m_lastPingReceived{0};
    uint32_t m_lastPongSent{0};
    uint32_t m_pendingServerTime{0};
    bool m_pongPending{false};

    uint32_t m_reactionDelayMs{0};

    [[nodiscard]] std::vector<uint8_t> EncodePongPacket() const;
};

} // namespace Client::Mimic
