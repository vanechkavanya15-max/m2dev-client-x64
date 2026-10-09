#pragma once

#include <span>
#include <cstdint>
#include <functional>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/PacketResult.h"
#include "../../Network/ModernPacketDispatcher.h"

namespace Client::Network {
    class HandshakeFlowAdapter;
}

namespace Client::Network::Handlers {

/**
 * @brief Obsluguje poczatkowy pakiet sieciowy Handshake.
 * 
 * Implementuje interfejs IPacketHandler do nowoczesnego dispatchingu pakietow.
 * Deleguje obsluge stanow i kryptografii do HandshakeFlowAdapter.
 */
class HandshakePacketHandler : public Client::Network::IPacketHandler {
public:
    using TimeProvider = std::function<uint32_t()>;

    /**
     * @brief Konstruktor.
     * @param adapter Adapter zarzadzajacy przeplywem handshake.
     * @param timeProvider Callback zwracajacy aktualny czas klienta w milisekundach.
     */
    HandshakePacketHandler(Client::Network::HandshakeFlowAdapter* adapter, TimeProvider timeProvider);
    ~HandshakePacketHandler() override = default;

    // Przesloniete metody IPacketHandler
    [[nodiscard]] EterBase::PacketResult<void> Handle(std::span<const uint8_t> payload) override;
    [[nodiscard]] uint16_t GetExpectedSize() const override;
    [[nodiscard]] bool IsDynamicSize() const override;

private:
    Client::Network::HandshakeFlowAdapter* m_adapter;
    TimeProvider m_timeProvider;
};

} // namespace Client::Network::Handlers
