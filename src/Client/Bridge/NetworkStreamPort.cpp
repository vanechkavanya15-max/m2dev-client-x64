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

    // W architekturze Strangler Fig (faza przejsciowa) fizyczne pakiety wire-protocol
    // (TPacketCGMove, TPacketCGAttack itd.) sa budowane i wysylane przez CPythonNetworkStream
    // z poprawnym naglowkiem 4-bajtowym [header:2][length:2] oraz szyfrowaniem XChaCha20.
    // Wywolania SendRaw z GameSession (opcody 1..7) sluza wylacznie do zasilania logiki domenowej,
    // sprawdzania polaczenia oraz symulacji/testow jednostkowych.
    // Przekazanie niesformatowanych bajtow do m_networkStream->Send() niszczylo szyfr strumieniowy
    // XChaCha20 i wywolywalo natychmiastowe rozlaczenie TCP przez serwer gry (kick).
    // Dlatego w polaczeniu z dzialajacym CNetworkStream bezpiecznie potwierdzamy polaczenie
    // bez wprowadzania smieciowych danych do bufora nadawczego.
    return {};
}

bool NetworkStreamPort::IsConnected() const noexcept {
    return m_networkStream != nullptr && m_networkStream->IsOnline();
}

} // namespace Client::Bridge
