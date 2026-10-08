#pragma once

#include <span>
#include <cstdint>
#include "../../EterBase/Result.h"

namespace Client::Core {
    class INetworkPort;
}

namespace Client::Network {

class HandshakeFSM;

/**
 * @brief Interface for managing multiple cryptographic states.
 * Created as a dependency for HandshakeFlowAdapter since it does not exist in the codebase.
 */
class MultiCryptoManager {
public:
    virtual ~MultiCryptoManager() = default;

    /**
     * @brief Transitions the cryptographic subsystem to a secure state.
     * @return Success or failure result.
     */
    virtual EterBase::VoidResult<> EnableSecureState() = 0;
};

/**
 * @brief Adapter for orchestrating the handshake and time synchronization flow.
 * Combines HandshakeFSM, MultiCryptoManager, and INetworkPort.
 */
class HandshakeFlowAdapter {
public:
    /**
     * @brief Constructs the HandshakeFlowAdapter.
     * @param fsm The finite state machine managing handshake phases.
     * @param cryptoManager The cryptographic manager to enable secure states.
     * @param networkPort The network port for sending raw packets.
     */
    HandshakeFlowAdapter(HandshakeFSM* fsm, MultiCryptoManager* cryptoManager, Core::INetworkPort* networkPort);

    ~HandshakeFlowAdapter() = default;

    // Delete copy/move semantics to enforce zero-conflict rules and single responsibility
    HandshakeFlowAdapter(const HandshakeFlowAdapter&) = delete;
    HandshakeFlowAdapter& operator=(const HandshakeFlowAdapter&) = delete;
    HandshakeFlowAdapter(HandshakeFlowAdapter&&) = delete;
    HandshakeFlowAdapter& operator=(HandshakeFlowAdapter&&) = delete;

    /**
     * @brief Handles an incoming handshake packet from the server.
     * Processes time synchronization, sends a response, and transitions to a secure state.
     * 
     * @param payload The raw byte payload of the packet.
     * @param currentClientTime The local client time in milliseconds.
     * @return PacketResult<> indicating success or failure.
     */
    [[nodiscard]] EterBase::PacketResult<void> HandleHandshakePacket(std::span<const uint8_t> payload, uint32_t currentClientTime);

private:
    HandshakeFSM* m_fsm;
    MultiCryptoManager* m_cryptoManager;
    Core::INetworkPort* m_networkPort;
};

} // namespace Client::Network
