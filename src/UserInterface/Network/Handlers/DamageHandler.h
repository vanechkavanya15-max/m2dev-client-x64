#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

namespace Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu otrzymanych lub zadanych obrazen (TPacketGCDamageInfo).
     */
    struct PacketDamageInfo
    {
        uint8_t header;
        uint32_t victimVid;
        uint8_t damageFlag;
        int32_t damageValue;
    };
#pragma pack(pop)

    /**
     * @brief Przetwarza pakiet obrazen postaci zwracajac scisly PacketResult C++23.
     * @param buffer Bufor bajtow ze strumienia sieciowego.
     * @return EterBase::PacketResult<void> wskazujacy sukces lub kod bledu PacketError.
     */
    EterBase::PacketResult<void> ProcessDamagePacket(std::span<const uint8_t> buffer);

    /**
     * @brief Przetwarza pakiet obrazen postaci (interfejs kompatybilny wstecz).
     * @param buffer Bufor bajtow ze strumienia sieciowego.
     * @return true w przypadku poprawnego odczytania pakietu.
     */
    bool HandleDamagePacket(std::span<const uint8_t> buffer);
}
