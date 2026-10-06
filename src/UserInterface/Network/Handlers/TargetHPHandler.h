#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

namespace Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu aktualizacji punktow zycia celu (TPacketGCTarget).
     */
    struct PacketTargetHP
    {
        uint8_t header;
        uint32_t targetVid;
        uint8_t hpPercentage;
    };
#pragma pack(pop)

    /**
     * @brief Przetwarza pakiet poziomu HP celu zwracajac PacketResult C++23.
     * @param buffer Bufor bajtow pakietu.
     * @return EterBase::PacketResult<void> ze scislym statusem sukcesu lub bledu.
     */
    EterBase::PacketResult<void> ProcessTargetHP(std::span<const uint8_t> buffer);

    /**
     * @brief Przetwarza pakiet aktualizacji poziomu HP celu (interfejs kompatybilny).
     * @param buffer Bufor bajtow pakietu.
     * @return true w przypadku poprawnego przetworzenia.
     */
    bool HandleTargetHP(std::span<const uint8_t> buffer);
}
