#ifndef TEST_MODE_DISABLE_STDAFX
#include "../../../EterBase/StdAfx.h"
#endif

#include "HandshakePacketHandler.h"
#include "../HandshakeFlowAdapter.h"
#include "../Protocol/Packets/Packet_Handshake.h"
#include "../../../EterBase/ModernLogger.h"

namespace Client::Network::Handlers {

HandshakePacketHandler::HandshakePacketHandler(Client::Network::HandshakeFlowAdapter* adapter, TimeProvider timeProvider)
    : m_adapter(adapter), m_timeProvider(std::move(timeProvider)) {
    if (!m_adapter) {
        EterBase::ModernLogger::Error("HandshakePacketHandler initialized with null adapter.");
    }
    if (!m_timeProvider) {
        EterBase::ModernLogger::Error("HandshakePacketHandler initialized with null timeProvider.");
    }
}

EterBase::PacketResult<void> HandshakePacketHandler::Handle(std::span<const uint8_t> payload) {
    if (!m_adapter) {
        EterBase::ModernLogger::Error("HandshakePacketHandler: HandshakeFlowAdapter is null.");
        return EterBase::MakeError(EterBase::PacketError::SessionClosed);
    }

    if (!m_timeProvider) {
        EterBase::ModernLogger::Error("HandshakePacketHandler: TimeProvider is null.");
        return EterBase::MakeError(EterBase::PacketError::SessionClosed);
    }

    if (payload.size() < GetExpectedSize()) {
        EterBase::ModernLogger::Error("HandshakePacketHandler: Payload too small.");
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    // Wywolanie adaptera do przetworzenia przeplywu handshake
    return m_adapter->HandleHandshakePacket(payload, m_timeProvider());
}

uint16_t HandshakePacketHandler::GetExpectedSize() const {
    return sizeof(PacketHandshake);
}

bool HandshakePacketHandler::IsDynamicSize() const {
    return false;
}

} // namespace Client::Network::Handlers
