#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

namespace Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu synchronizacji pozycji postaci (TPacketGCPosition).
     */
    struct PacketCharPosition
    {
        uint16_t header;
        uint16_t length;
        uint32_t characterVid;
        uint8_t  position;
    };
#pragma pack(pop)

    /**
     * @brief Aktualizuje pozycje postaci zwracajac PacketResult C++23.
     * @param buffer Bufor bajtow ze strumienia.
     * @return EterBase::PacketResult<void> ze statusem powodzenia lub bledu.
     */
    EterBase::PacketResult<void> ProcessCharacterPosition(std::span<const uint8_t> buffer);

    /**
     * @brief Aktualizuje pozycje postaci na mapie (interfejs kompatybilny).
     * @param buffer Bufor bajtow ze strumienia.
     * @return true w przypadku powodzenia.
     */
    bool HandleCharacterPosition(std::span<const uint8_t> buffer);
}
