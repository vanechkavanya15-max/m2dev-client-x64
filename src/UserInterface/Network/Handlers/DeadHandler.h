#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

namespace Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu oznaczajacego smierc encji (wczesniej TPacketGCDead).
     */
    struct PacketDead
    {
        uint16_t header;
        uint16_t length;
        uint32_t targetId;
    };
#pragma pack(pop)

    /**
     * @brief Zglasza smierc postaci zwracajac PacketResult C++23.
     * @param buffer Bufor z danymi pakietu.
     * @return EterBase::PacketResult<void> wskazujacy sukces lub kod bledu PacketError.
     */
    EterBase::PacketResult<void> ProcessDeadPacket(std::span<const uint8_t> buffer);

    /**
     * @brief Zglasza smierc postaci na podstawie pakietu sieciowego (interfejs kompatybilny).
     * @param buffer Bufor z danymi pakietu.
     * @return Zwraca true w przypadku poprawnego przetworzenia pakietu, w przeciwnym razie false.
     */
    bool HandleDeadPacket(std::span<const uint8_t> buffer);
}
