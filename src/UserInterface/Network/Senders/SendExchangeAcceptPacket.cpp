#include "StdAfx.h"
#include "SendExchangeAcceptPacket.h"
#include "../../PythonNetworkStream.h"
#include "../../Packet.h"
#include <EterBase/LogModern.h>
#include <EterBase/Result.h>
#include <span>

/**
 * @file SendExchangeAcceptPacket.cpp
 * @brief Implementacja wysylania pakietu akceptacji handlu w C++23.
 * 
 * Ten plik jest czescia inicjatywy modernizacji C++23.
 */

namespace Network::Senders {

    /**
     * @brief Wysyla zadanie akceptacji wymiany handlowej do serwera.
     * 
     * @param stream Referencja do strumienia sieciowego.
     * @return EterBase::PacketResult<void> Sukces jezeli wyslanie sie powiodlo, w przeciwnym razie blad.
     */
    EterBase::PacketResult<void> SendExchangeAcceptPacket(CPythonNetworkStream& stream)
    {
        TPacketCGExchange packet{};
        packet.header = CG::EXCHANGE;
        packet.length = static_cast<uint16_t>(sizeof(packet));
        packet.subheader = ExchangeSub::CG::ACCEPT;
        
        std::span<const uint8_t> packetSpan{ reinterpret_cast<const uint8_t*>(&packet), sizeof(packet) };

        if (!stream.Send(static_cast<int>(packetSpan.size()), packetSpan.data()))
        {
            EterBase::ModernLogger::Error("SendExchangeAcceptPacket Error: Failed to send packet");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        return {};
    }

} // namespace Network::Senders
