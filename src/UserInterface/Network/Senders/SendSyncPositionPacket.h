#pragma once

#include <span>
#include <cstdint>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

class CPythonNetworkStream;

namespace UserInterface::Network {

    /**
     * @brief Struktura przechowujaca cel do synchronizacji pozycji w domenie C++23.
     */
    struct SyncPositionTarget {
        EterBase::EntityId victimId;
        int32_t x;
        int32_t y;
    };

    /**
     * @brief Wysyla pakiet korekty synchronizacji pozycji (zbiorczy) do serwera (C++23).
     * 
     * @param stream Referencja do glownego strumienia sieciowego.
     * @param targets Lista celow do synchronizacji (std::span).
     * @return EterBase::PacketResult<void> ze statusem operacji.
     */
    EterBase::PacketResult<void> SendSyncPositionPacket(CPythonNetworkStream& stream, std::span<const SyncPositionTarget> targets);

} // namespace UserInterface::Network
