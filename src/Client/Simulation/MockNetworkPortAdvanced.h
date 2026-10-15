#pragma once

#include "../Core/INetworkPort.h"
#include <vector>
#include <functional>
#include <unordered_map>
#include <optional>

namespace Client::Simulation {

struct PacketData {
    uint8_t opcode;
    std::vector<uint8_t> payload;
};

class MockNetworkPortAdvanced : public Core::INetworkPort {
public:
    using AutoResponderCallback = std::function<void(const std::span<const uint8_t>&)>;
    using ReceiveCallback = std::function<void(uint8_t, const std::span<const uint8_t>&)>;

    MockNetworkPortAdvanced();
    ~MockNetworkPortAdvanced() override = default;

    // INetworkPort implementation
    [[nodiscard]] Core::Result<void, Core::PacketError> SendRaw(uint8_t opcode, std::span<const uint8_t> payload) override;
    [[nodiscard]] bool IsConnected() const noexcept override;

    // Recording features
    [[nodiscard]] const std::vector<PacketData>& GetSentPackets() const noexcept;
    [[nodiscard]] size_t CountSentPackets(uint8_t opcode) const noexcept;
    [[nodiscard]] std::optional<uint8_t> GetLastOpcode() const noexcept;
    void ClearSentPackets();

    // Injection features
    void InjectPacket(uint8_t opcode, std::span<const uint8_t> payload);
    void SetReceiveCallback(ReceiveCallback callback);
    [[nodiscard]] const std::vector<PacketData>& GetInjectedPackets() const noexcept;
    void ClearInjectedPackets();

    // Auto-Responder features
    void RegisterAutoResponder(uint8_t opcode, AutoResponderCallback callback);
    void ClearAutoResponders();

    // Connection state simulation
    void SetConnected(bool connected) noexcept;

private:
    bool m_connected{true};
    std::vector<PacketData> m_sentPackets;
    std::vector<PacketData> m_injectedPackets;
    
    std::unordered_map<uint8_t, AutoResponderCallback> m_autoResponders;
    ReceiveCallback m_receiveCallback;
};

} // namespace Client::Simulation
