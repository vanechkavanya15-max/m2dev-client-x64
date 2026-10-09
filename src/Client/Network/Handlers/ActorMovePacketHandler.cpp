#include "ActorMovePacketHandler.h"
#include <Client/Network/Protocol/Protocol.h>
#include <EterBase/ModernLogger.h>
#include <cstring>

namespace Client::Network::Handlers {

// Typowe flagi uzywane w TPacketGCMove (0 = WAIT, 1 = MOVE)
constexpr uint8_t FUNC_WAIT = 0;
constexpr uint8_t FUNC_MOVE = 1;

EterBase::PacketResult<void> ActorMovePacketHandler::HandleActorMove(
    std::span<const uint8_t> payload, 
    Client::World::SpatialHashGrid& grid) const noexcept 
{
    // Bezpieczne sprawdzenie granic przed kopiowaniem (Memory Safety)
    if (payload.size() < sizeof(TPacketGCMove)) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCMove packet;
    // Bezpieczne rzutowanie przez std::memcpy, poniewaz struktura pakiety moze nie byc prawidlowo wyrownana
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCMove));

    // Walidacja prawidlowosci pola bFunc
    if (packet.bFunc != FUNC_WAIT && packet.bFunc != FUNC_MOVE) {
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    EterBase::EntityId vid{packet.dwVID};
    float x = static_cast<float>(packet.lX);
    float y = static_cast<float>(packet.lY);
    // rotacja bRot jest aktualnie ignorowana dla SpatialHashGrid, ale bedzie prawidlowo zdeserializowana

    grid.Update(vid, x, y);

    return {};
}

} // namespace Client::Network::Handlers
