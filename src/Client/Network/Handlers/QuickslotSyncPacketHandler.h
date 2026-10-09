#pragma once

#include <span>
#include <cstdint>
#include "../../../EterBase/Result.h"

namespace Client::Gameplay {
    class QuickslotDomain;
}

namespace Client::Network::Handlers {

/**
 * @class QuickslotSyncPacketHandler
 * @brief Handler odpowiedzialny za parsowanie i procesowanie pakietow synchronizacji paska szybkiego dostepu
 *
 * Zgodnie z zasada Zero-Conflict z C++23:
 * - Odbiera buffer za pomoca span (bez kopiowania).
 * - Deserializuje pakiety GC::QUICKSLOT_ADD, GC::QUICKSLOT_DEL, GC::QUICKSLOT_SWAP.
 * - Wywoluje odpowiednie metody z QuickslotDomain.
 * - Zwraca EterBase::PacketResult<void>
 */
class QuickslotSyncPacketHandler {
public:
    /**
     * @brief Przetwarza pakiet QUICKSLOT_ADD z podanego bufora.
     * @param buffer Bufor bajtow zawierajacy TPacketGCQuickSlotAdd.
     * @param quickslotDomain Referencja do domeny paska szybkiego dostepu.
     * @return Zwraca EterBase::PacketResult<void> na zakonczenie procesu.
     */
    static EterBase::PacketResult<void> HandleAdd(std::span<const uint8_t> buffer, Client::Gameplay::QuickslotDomain& quickslotDomain) noexcept;

    /**
     * @brief Przetwarza pakiet QUICKSLOT_DEL z podanego bufora.
     * @param buffer Bufor bajtow zawierajacy TPacketGCQuickSlotDel.
     * @param quickslotDomain Referencja do domeny paska szybkiego dostepu.
     * @return Zwraca EterBase::PacketResult<void> na zakonczenie procesu.
     */
    static EterBase::PacketResult<void> HandleDel(std::span<const uint8_t> buffer, Client::Gameplay::QuickslotDomain& quickslotDomain) noexcept;

    /**
     * @brief Przetwarza pakiet QUICKSLOT_SWAP z podanego bufora.
     * @param buffer Bufor bajtow zawierajacy TPacketGCQuickSlotSwap.
     * @param quickslotDomain Referencja do domeny paska szybkiego dostepu.
     * @return Zwraca EterBase::PacketResult<void> na zakonczenie procesu.
     */
    static EterBase::PacketResult<void> HandleSwap(std::span<const uint8_t> buffer, Client::Gameplay::QuickslotDomain& quickslotDomain) noexcept;
};

} // namespace Client::Network::Handlers
