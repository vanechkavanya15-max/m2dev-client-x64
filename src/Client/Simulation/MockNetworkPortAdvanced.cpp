#include "MockNetworkPortAdvanced.h"

namespace Client::Simulation {

MockNetworkPortAdvanced::MockNetworkPortAdvanced() = default;

Core::Result<void, Core::PacketError> MockNetworkPortAdvanced::SendRaw(uint8_t opcode, std::span<const uint8_t> payload) {
    if (!m_connected) {
        return std::unexpected(Core::PacketError::Timeout);
    }

    std::vector<uint8_t> payloadCopy(payload.begin(), payload.end());
    m_sentPackets.push_back({opcode, payloadCopy});

    if (auto it = m_autoResponders.find(opcode); it != m_autoResponders.end()) {
        it->second(payload);
    }

    return {};
}

bool MockNetworkPortAdvanced::IsConnected() const noexcept {
    return m_connected;
}

const std::vector<PacketData>& MockNetworkPortAdvanced::GetSentPackets() const noexcept {
    return m_sentPackets;
}

size_t MockNetworkPortAdvanced::CountSentPackets(uint8_t opcode) const noexcept {
    size_t count = 0;
    for (const auto& packet : m_sentPackets) {
        if (packet.opcode == opcode) {
            count++;
        }
    }
    return count;
}

std::optional<uint8_t> MockNetworkPortAdvanced::GetLastOpcode() const noexcept {
    if (m_sentPackets.empty()) {
        return std::nullopt;
    }
    return m_sentPackets.back().opcode;
}

void MockNetworkPortAdvanced::ClearSentPackets() {
    m_sentPackets.clear();
}

void MockNetworkPortAdvanced::InjectPacket(uint8_t opcode, std::span<const uint8_t> payload) {
    std::vector<uint8_t> payloadCopy(payload.begin(), payload.end());
    m_injectedPackets.push_back({opcode, payloadCopy});

    if (m_receiveCallback) {
        m_receiveCallback(opcode, payload);
    }
}

void MockNetworkPortAdvanced::SetReceiveCallback(ReceiveCallback callback) {
    m_receiveCallback = std::move(callback);
}

const std::vector<PacketData>& MockNetworkPortAdvanced::GetInjectedPackets() const noexcept {
    return m_injectedPackets;
}

void MockNetworkPortAdvanced::ClearInjectedPackets() {
    m_injectedPackets.clear();
}

void MockNetworkPortAdvanced::RegisterAutoResponder(uint8_t opcode, AutoResponderCallback callback) {
    m_autoResponders[opcode] = std::move(callback);
}

void MockNetworkPortAdvanced::ClearAutoResponders() {
    m_autoResponders.clear();
}

void MockNetworkPortAdvanced::SetConnected(bool connected) noexcept {
    m_connected = connected;
}

} // namespace Client::Simulation
