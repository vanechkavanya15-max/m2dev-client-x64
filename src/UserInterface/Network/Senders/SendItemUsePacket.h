#pragma once

#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../GameType.h"

namespace Network::Senders
{
    /**
     * @brief Zglasza do serwera zadanie uzycia przedmiotu przez gracza z weryfikacja biezacej pozycji w ekwipunku.
     * 
     * Nowoczesna metoda zgodna z C++23. Funkcja wykorzystuje system EterBase::PacketResult
     * do bezpiecznego zwracania statusow powodzenia oraz kodow bledow.
     * 
     * @param slot Pozycja przedmiotu w ekwipunku gracza (silny typ ItemSlot).
     * @return EterBase::PacketResult<void> Pusty rezultat w przypadku sukcesu lub PacketError w przypadku niepowodzenia.
     */
    EterBase::PacketResult<void> SendItemUsePacket(EterBase::ItemSlot slot);
}
