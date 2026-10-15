#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

namespace Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu informacji o wierzchowcu (GC_MOUNT).
     * 
     * Odpowiada oryginalnej strukturze TPacketGCMount. 
     * Uzywa scislego wyrownania (1-byte alignment).
     */
    struct PacketMount
    {
        uint16_t header;
        uint16_t length;
        uint32_t vid;
        uint32_t mount_vid;
        uint8_t pos;
        uint32_t _x;
        uint32_t _y;
    };
#pragma pack(pop)

    /**
     * @brief Przetwarza pakiet obslugi wierzchowca zwracajac PacketResult C++23.
     * @param buffer Bufor bajtow ze strumienia.
     * @return EterBase::PacketResult<void> ze statusem powodzenia lub bledu.
     */
    EterBase::PacketResult<void> ProcessMountPacket(std::span<const uint8_t> buffer);

    /**
     * @brief Aktualizuje informacje o wierzchowcu postaci (interfejs kompatybilny).
     * @param buffer Bufor bajtow ze strumienia.
     * @return true w przypadku poprawnego przetworzenia.
     */
    bool HandleMountPacket(std::span<const uint8_t> buffer);
}
