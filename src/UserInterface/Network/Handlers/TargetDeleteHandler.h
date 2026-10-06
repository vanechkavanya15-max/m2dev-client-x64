#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

namespace Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu usuwajacego znacznik celu (TPacketGCTargetDelete).
     */
    struct PacketTargetDelete
    {
        uint16_t header;
        uint16_t length;
        int32_t targetId;
    };
#pragma pack(pop)

    /**
     * @brief Przetwarza pakiet usuniecia celu zwracajac PacketResult C++23.
     * @param buffer Bufor bajtow pakietu.
     * @return EterBase::PacketResult<void> ze scislym statusem sukcesu lub bledu.
     */
    EterBase::PacketResult<void> ProcessTargetDelete(std::span<const uint8_t> buffer);

    /**
     * @brief Przetwarza pakiet usuniecia celu (interfejs kompatybilny).
     * @param buffer Bufor bajtow pakietu.
     * @return true w przypadku poprawnego przetworzenia.
     */
    bool HandleTargetDelete(std::span<const uint8_t> buffer);
}
