#pragma once

#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

// Forward declaration by uniknac inkludowania gigantycznych naglowkow (PythonNetworkStream.h)
class CPythonNetworkStream;

namespace Network::Senders
{
    /**
     * @brief Wysyla pakiet klikniecia w cel do serwera (HEADER_CG_ON_CLICK).
     * 
     * Wykorzystywane gdy gracz wchodzi w interakcje z obiektem, takim jak NPC, mob czy brama.
     * Implementacja z uzyciem std::expected. Funkcja bezposrednio wpisuje payload pakietu.
     * Nie wywoluje wirtualnych funkcji, dziala event-driven.
     * 
     * @param stream Referencja do instancji strumienia sieciowego klienta.
     * @param targetId Typ silny (EntityId) docelowego obiektu (VID) kliknietego przez gracza.
     * @return EterBase::PacketResult<void> Pusty stan sukcesu (has_value() == true) albo zdefiniowany kod bledu domeny (PacketError).
     */
    EterBase::PacketResult<void> SendOnClickPacket(CPythonNetworkStream& stream, EterBase::EntityId targetId);
}
