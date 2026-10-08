#include "StdAfx.h"
#include "NetworkStreamPort.h"
#include "EterLib/NetStream.h"
#include <vector>

namespace Client::Bridge {

NetworkStreamPort::NetworkStreamPort(CNetworkStream* networkStream) noexcept
    : m_networkStream(networkStream)
{
}

Client::Core::Result<void, Client::Core::PacketError> NetworkStreamPort::SendRaw(
    uint8_t opcode,
    std::span<const uint8_t> payload)
{
    if (!m_networkStream || !m_networkStream->IsOnline()) {
        return std::unexpected(Client::Core::PacketError::Timeout);
    }

    if (payload.empty()) {
        if (!m_networkStream->Send(sizeof(opcode), &opcode)) {
            return std::unexpected(Client::Core::PacketError::BufferUnderflow);
        }
        return {};
    }

    // Jeśli w pierwszym bajcie payloadu jest już ten sam opcode, wysyłamy bezpośrednio
    if (payload.front() == opcode) {
        if (!m_networkStream->Send(static_cast<int>(payload.size()), payload.data())) {
            return std::unexpected(Client::Core::PacketError::BufferUnderflow);
        }
        return {};
    }

    // W przeciwnym razie składamy nagłówek z opcode i payload
    std::vector<uint8_t> buffer;
    buffer.reserve(1 + payload.size());
    buffer.push_back(opcode);
    buffer.insert(buffer.end(), payload.begin(), payload.end());

    if (!m_networkStream->Send(static_cast<int>(buffer.size()), buffer.data())) {
        return std::unexpected(Client::Core::PacketError::BufferUnderflow);
    }

    return {};
}

bool NetworkStreamPort::IsConnected() const noexcept {
    return m_networkStream != nullptr && m_networkStream->IsOnline();
}

} // namespace Client::Bridge
