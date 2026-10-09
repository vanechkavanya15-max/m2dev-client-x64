#pragma once

#include <span>
#include <cstdint>
#include <EterBase/Result.h>
#include <EterBase/StrongTypes.h>
#include <Client/World/SpatialHashGrid.h>

namespace Client::Network::Handlers {

/**
 * @brief Nowoczesny handler obslugujacy pakiety ruchu (TPacketGCMove) w oparciu o C++20 std::span.
 * Implementuje paradygmat Zero-Conflict oraz zapewnia pelne bezpieczenstwo pamieci (Memory Safety).
 */
class ActorMovePacketHandler {
public:
    ActorMovePacketHandler() = default;
    ~ActorMovePacketHandler() = default;

    /**
     * @brief Przetwarza pakiet ruchu i aktualizuje pozycje aktora w siatce kolizji (SpatialHashGrid).
     * @param payload Bufor z danymi pakietu z gwarancja bezpieczenstwa granic (bounds checking).
     * @param grid Referencja do siatki przestrzennej, do ktorej zostanie zgloszona nowa pozycja.
     * @return Sukces (void) lub blad (np. BufferUnderflow, MalformedPayload).
     */
    EterBase::PacketResult<void> HandleActorMove(
        std::span<const uint8_t> payload, 
        Client::World::SpatialHashGrid& grid) const noexcept;
};

} // namespace Client::Network::Handlers
