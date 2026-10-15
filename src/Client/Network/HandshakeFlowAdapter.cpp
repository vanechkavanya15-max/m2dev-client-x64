#include "../../EterBase/StdAfx.h"
#include "HandshakeFlowAdapter.h"
#include "HandshakeFSM.h"
#include "../Core/INetworkPort.h"
#include "Protocol/Protocol.h"
#include "Protocol/Packets/Packet_Handshake.h"
#include "../../EterBase/ModernLogger.h"
#include <cstring>

namespace Client::Network {

HandshakeFlowAdapter::HandshakeFlowAdapter(HandshakeFSM* fsm, MultiCryptoManager* cryptoManager, Core::INetworkPort* networkPort)
    : m_fsm(fsm), m_cryptoManager(cryptoManager), m_networkPort(networkPort) {
}

[[nodiscard]] EterBase::PacketResult<void> HandshakeFlowAdapter::HandleHandshakePacket(std::span<const uint8_t> payload, uint32_t currentClientTime) {
    if (!m_fsm || !m_cryptoManager || !m_networkPort) {
        EterBase::ModernLogger::Error("HandshakeFlowAdapter: Uninitialized dependencies.");
        return EterBase::MakeError(EterBase::PacketError::SessionClosed);
    }

    if (payload.size() < sizeof(PacketHandshake)) {
        EterBase::ModernLogger::Error("HandshakeFlowAdapter: Payload too small for PacketHandshake.");
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    PacketHandshake packet;
    std::memcpy(&packet, payload.data(), sizeof(PacketHandshake));

    // Transition FSM to HandshakeReceived
    auto transition1 = m_fsm->TransitionToHandshakeReceived(currentClientTime, packet.time, packet.delta);
    if (!transition1) {
        EterBase::ModernLogger::Error("HandshakeFlowAdapter: Failed to transition to HandshakeReceived: {}", transition1.error());
        return EterBase::MakeError(EterBase::PacketError::SequenceMismatch);
    }

    // Calculate compensated server time
    uint32_t compensatedServerTime = packet.time + packet.delta;

    // Prepare response packet
    PacketHandshake responsePacket = packet;
    responsePacket.time = compensatedServerTime;

    std::span<const uint8_t> responseSpan(reinterpret_cast<const uint8_t*>(&responsePacket), sizeof(PacketHandshake));
    
    // Send raw response (time sync sent)
    auto sendResult = m_networkPort->SendRaw(responsePacket.header, responseSpan);
    if (!sendResult) {
        EterBase::ModernLogger::Error("HandshakeFlowAdapter: Failed to send Handshake sync response.");
        return EterBase::MakeError(EterBase::PacketError::SessionClosed);
    }

    // Transition FSM to TimeSyncSent
    auto transition2 = m_fsm->TransitionToTimeSyncSent();
    if (!transition2) {
        EterBase::ModernLogger::Error("HandshakeFlowAdapter: Failed to transition to TimeSyncSent: {}", transition2.error());
        return EterBase::MakeError(EterBase::PacketError::SequenceMismatch);
    }

    // Enable secure state
    auto cryptoResult = m_cryptoManager->EnableSecureState();
    if (!cryptoResult) {
        EterBase::ModernLogger::Error("HandshakeFlowAdapter: Failed to enable secure state: {}", cryptoResult.error());
        return EterBase::MakeError(EterBase::PacketError::SessionClosed);
    }

    // Complete the Handshake
    auto transition3 = m_fsm->TransitionToComplete();
    if (!transition3) {
        EterBase::ModernLogger::Error("HandshakeFlowAdapter: Failed to transition to Complete: {}", transition3.error());
        return EterBase::MakeError(EterBase::PacketError::SequenceMismatch);
    }

    return {};
}

} // namespace Client::Network
