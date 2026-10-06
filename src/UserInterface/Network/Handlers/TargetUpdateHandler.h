#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

namespace Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu aktualizacji znacznika misji (TPacketGCTargetUpdate).
     */
    struct PacketTargetUpdate
    {
        uint16_t header;
        uint16_t length;
        int32_t targetId;
        int32_t x;
        int32_t y;
    };
#pragma pack(pop)

    /**
     * @brief Przetwarza pakiet aktualizacji pozycji znacznika misji (TargetUpdate) zwracajac PacketResult C++23.
     * @param buffer Bufor bajtow pakietu.
     * @return EterBase::PacketResult<void> ze scislym statusem sukcesu lub bledu.
     */
    EterBase::PacketResult<void> ProcessTargetUpdate(std::span<const uint8_t> buffer);

    /**
     * @brief Przetwarza pakiet aktualizacji pozycji znacznika misji (TargetUpdate) (interfejs kompatybilny).
     * @param buffer Bufor bajtow pakietu.
     * @return true w przypadku poprawnego przetworzenia.
     */
    bool HandleTargetUpdate(std::span<const uint8_t> buffer);
}
