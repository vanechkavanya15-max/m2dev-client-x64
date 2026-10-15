#include "PingPongEngine.h"
#include <cstring>

namespace Client::Mimic {

void PingPongEngine::SetProfile(const Network::ServerProfile* profile) {
    m_profile = profile;
    if (m_profile) {
        m_reactionDelayMs = m_profile->GetMoveInterval() > 0 ? m_profile->GetMoveInterval() : 100;
    }
}

EterBase::PacketResult<void> PingPongEngine::ProcessPing(std::span<const uint8_t> payload, uint32_t currentClientTime) {
    if (!m_profile) {
        return EterBase::MakeError(EterBase::PacketError::SessionClosed);
    }

    size_t expectedHeaderSize = m_profile->GetHeaderSizeBytes();
    size_t expectedLengthSize = (m_profile->GetFramingMode() == Network::FramingMode::M2Dev4B) ? 4 : 2;
    size_t expectedTimeSize = 4;
    
    size_t totalExpectedSize = expectedHeaderSize + expectedLengthSize + expectedTimeSize;

    if (payload.size() < totalExpectedSize) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    size_t offset = expectedHeaderSize + expectedLengthSize;
    uint32_t serverTime = 0;
    std::memcpy(&serverTime, payload.data() + offset, sizeof(serverTime));

    m_lastPingReceived = currentClientTime;
    m_pendingServerTime = serverTime;
    m_pongPending = true;

    return {};
}

std::vector<uint8_t> PingPongEngine::Update(uint32_t currentClientTime) {
    std::vector<uint8_t> pongData;

    if (!m_profile || !m_pongPending) {
        return pongData;
    }

    if (currentClientTime >= m_lastPingReceived + m_reactionDelayMs) {
        pongData = EncodePongPacket();
        m_lastPongSent = currentClientTime;
        m_pongPending = false;
    }

    return pongData;
}

std::vector<uint8_t> PingPongEngine::EncodePongPacket() const {
    std::vector<uint8_t> buf;
    if (!m_profile) return buf;

    auto opcodeResult = m_profile->GetOpcode("CG_PONG");
    uint8_t opcode = opcodeResult.has_value() ? opcodeResult.value() : 0x40;

    uint8_t headerSize = m_profile->GetHeaderSizeBytes();
    
    if (headerSize == 1) {
        buf.push_back(opcode);
    } else {
        buf.push_back(opcode);
        buf.push_back(0x00);
    }

    bool isM2Dev4B = (m_profile->GetFramingMode() == Network::FramingMode::M2Dev4B);
    uint32_t packetLength = headerSize + (isM2Dev4B ? 4 : 2);

    if (isM2Dev4B) {
        buf.push_back(static_cast<uint8_t>(packetLength & 0xFF));
        buf.push_back(static_cast<uint8_t>((packetLength >> 8) & 0xFF));
        buf.push_back(static_cast<uint8_t>((packetLength >> 16) & 0xFF));
        buf.push_back(static_cast<uint8_t>((packetLength >> 24) & 0xFF));
    } else {
        buf.push_back(static_cast<uint8_t>(packetLength & 0xFF));
        buf.push_back(static_cast<uint8_t>((packetLength >> 8) & 0xFF));
    }

    return buf;
}

HeartbeatStatus PingPongEngine::GetHeartbeatStatus(uint32_t currentClientTime) const {
    if (m_lastPingReceived == 0) {
        return HeartbeatStatus::Disconnected;
    }

    uint32_t elapsed = currentClientTime - m_lastPingReceived;

    if (elapsed >= 15000) {
        return HeartbeatStatus::Disconnected;
    } else if (elapsed >= 5000) {
        return HeartbeatStatus::Degraded;
    }

    return HeartbeatStatus::Healthy;
}

} // namespace Client::Mimic
