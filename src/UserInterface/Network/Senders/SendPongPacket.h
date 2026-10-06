#pragma once

#include "../../../EterBase/Result.h"

class CNetworkStream;

namespace Network::Senders {

/**
 * @class PongSender
 * @brief Odpowiada za wyslanie pakietu PONG do serwera (odpowiedz na PING / keep-alive).
 * 
 * Klasa implementuje zasade SRP, separujac wysylanie odpowiedzi PONG 
 * od zlozonych zaleznosci sieciowych i interfejsu uzytkownika. 
 * Korzysta z monadycznego `std::expected` (poprzez EterBase::PacketResult) 
 * w celu nowoczesnej, deterministycznej obslugi bledow zgodnie ze standardem C++23.
 */
class PongSender {
public:
    /**
     * @brief Formatuje i wysyla odpowiedz PONG do serwera.
     * 
     * Wykorzystuje dostarczony strumien sieciowy aby nadac pakiet typu `TPacketCGPong`.
     * Chroni przed wyslaniem na pustym polaczeniu.
     * 
     * @param networkStream Wskaznik na aktualny, otwarty strumien sieciowy (CNetworkStream).
     * @return EterBase::PacketResult<void> Zwraca sukces (puste) jesli wyslano, 
     *         lub SessionClosed jesli polaczenie jest nieaktywne albo wysylka sie nie powiodla.
     */
    static EterBase::PacketResult<void> Send(CNetworkStream* networkStream);
};

} // namespace Network::Senders
