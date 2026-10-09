#pragma once

#include <span>
#include <cstdint>
#include "EterBase/Result.h"
#include "Client/Network/PendingSpawnRegistry.h"
#include "Client/Network/ActorPacketCodec.h"

namespace Client::Network::Handlers {

    /**
     * @brief Klasa obslugujaca parsowanie pakietu rozszerzonych informacji o aktorze (0x46).
     * 
     * Komponent ten wyciaga dodatkowe informacje spolecznosciowe (np. nazwa, gildia, ranga,
     * tryb PvP, poziom) ze strumienia danych i przekazuje je do PendingSpawnRegistry,
     * aby umozliwic dokonczenie procesu spawnowania. Dziala zgodnie z zasada Zero-Conflict.
     */
    class ActorAdditionalInfoPacketHandler {
    public:
        /**
         * @brief Analizuje pakiet HEADER_GC_CHAR_ADDITIONAL_INFO i rejestruje dane spawnu.
         * 
         * @param payload Binarny ciag danych zawierajacy pakiet TPacketGCCharacterAdditionalInfo.
         * @param registry Referencja do IPendingSpawnRegistry.
         * @return EterBase::PacketResult<void> Sukces operacji lub blad parsowania/rejestracji.
         */
        static EterBase::PacketResult<void> Handle(std::span<const uint8_t> payload, Client::Network::IPendingSpawnRegistry& registry);
    };

} // namespace Client::Network::Handlers
