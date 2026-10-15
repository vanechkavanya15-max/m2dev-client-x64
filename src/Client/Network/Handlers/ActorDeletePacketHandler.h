#pragma once

#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../ActorPacketCodec.h"
#include "../../World/SpatialHashGrid.h"
#include "../PendingSpawnRegistry.h"

namespace Client::Network::Handlers {

    /**
     * @class ActorDeletePacketHandler
     * @brief Nowoczesny handler obslugujacy usuwanie bytow (despawn) w standardzie AI-First.
     *
     * Odpowiada za:
     * - Odkodowanie i walidacje pakietu TPacketGCCharacterDelete.
     * - Usuwanie bytu ze struktur przestrzennych (SpatialHashGrid).
     * - Anulowanie ew. oczekujacych spawnow (PendingSpawnRegistry) 
     *   oraz bezpieczne usuwanie sierocych stanow posrednich (dane szczatkowe spawnu).
     */
    class ActorDeletePacketHandler {
    public:
        ActorDeletePacketHandler() = delete;

        /**
         * @brief Przetwarza pakiet usuniecia aktora ze swiata.
         * 
         * @param packet Referencja do surowych danych pakietu usuniecia z sieci.
         * @param spatialGrid Referencja do struktury SpatialHashGrid, z ktorej usuniety bedzie byt.
         * @param spawnRegistry Rejestr oczekujacych spawnow pozwalajacy na abortowanie procesu.
         * @return EterBase::PacketResult<void> Wynik wykonania operacji: Void w przypadku sukcesu, blad sieci w przeciwnym razie.
         */
        [[nodiscard]] static EterBase::PacketResult<void> Handle(
            const Client::Network::TPacketGCCharacterDelete& packet,
            Client::World::SpatialHashGrid& spatialGrid,
            Client::Network::IPendingSpawnRegistry& spawnRegistry
        ) noexcept {
            auto decodeResult = Client::Network::ActorPacketCodec::DecodeCharacterDelete(packet);
            if (!decodeResult) {
                return EterBase::MakeError(decodeResult.error());
            }

            const EterBase::EntityId vid = decodeResult.value();

            // Bezpieczne i bezwarunkowe usuniecie ze SpatialHashGrid
            spatialGrid.Remove(vid);

            // Pobranie/usuniecie bytu z rejestru oczekujacych spawnow
            auto spawnResult = spawnRegistry.TakeSpawn(vid);
            
            // Jezeli byt ma "invalid type" oznacza to, ze utkwil w stanie polowicznego spawnu
            // (wylacznie paczka podstawowa z ActorSpawn bez AdditionalInfo).
            // Nalezy sprytnie ominac to dorzucajac mu puste dodatkowe dane i zdejmujac ponownie.
            if (!spawnResult && spawnResult.error() == EterBase::EntityError::InvalidType) {
                Client::Network::TPacketGCCharacterAdditionalInfo dummyInfo{};
                dummyInfo.dwVID = vid.get();
                
                // Rejestrujemy falszywe info aby zaspokoic wymogi TakeSpawn
                if (spawnRegistry.RegisterAdditionalInfo(vid, dummyInfo)) {
                    (void)spawnRegistry.TakeSpawn(vid);
                }
            }

            return {};
        }
    };

} // namespace Client::Network::Handlers
