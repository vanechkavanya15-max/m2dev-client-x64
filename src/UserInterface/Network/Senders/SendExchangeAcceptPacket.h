#pragma once

#include <EterBase/Result.h>

class CPythonNetworkStream;

namespace Network::Senders {

    /**
     * @brief Wysyla zadanie akceptacji wymiany handlowej do serwera.
     * 
     * @param stream Referencja do strumienia sieciowego.
     * @return EterBase::PacketResult<void> Sukces jezeli wyslanie sie powiodlo, w przeciwnym razie blad.
     */
    EterBase::PacketResult<void> SendExchangeAcceptPacket(CPythonNetworkStream& stream);

} // namespace Network::Senders
