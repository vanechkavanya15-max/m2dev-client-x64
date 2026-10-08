#pragma once

#include "Client/Core/INetworkPort.h"
#include <cstdint>
#include <span>

class CNetworkStream;

namespace Client::Bridge {

class NetworkStreamPort : public Client::Core::INetworkPort {
public:
    explicit NetworkStreamPort(CNetworkStream* networkStream) noexcept;
    ~NetworkStreamPort() override = default;

    NetworkStreamPort(const NetworkStreamPort&) = delete;
    NetworkStreamPort& operator=(const NetworkStreamPort&) = delete;
    NetworkStreamPort(NetworkStreamPort&&) noexcept = default;
    NetworkStreamPort& operator=(NetworkStreamPort&&) noexcept = default;

    [[nodiscard]] Client::Core::Result<void, Client::Core::PacketError> SendRaw(
        uint8_t opcode,
        std::span<const uint8_t> payload) override;

    [[nodiscard]] bool IsConnected() const noexcept override;

    void SetNetworkStream(CNetworkStream* stream) noexcept { m_networkStream = stream; }

private:
    CNetworkStream* m_networkStream{nullptr};
};

} // namespace Client::Bridge
