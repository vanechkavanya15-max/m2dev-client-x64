#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

namespace Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu synchronizacji pozycji postaci (TPacketGCCharacterPosition).
     */
    struct PacketCharPosition
    {
        uint8_t header;
        uint32_t characterVid;
        int32_t x;
        int32_t y;
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
