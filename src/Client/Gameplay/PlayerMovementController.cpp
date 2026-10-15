#include "PlayerMovementController.h"
#include "Client/Network/MovementCommandEncoder.h"

namespace Client::Gameplay {

PlayerMovementController::PlayerMovementController(const ITerrainAttributeProvider* provider,
                                                 MovementRateLimiter::Duration limitInterval)
    : m_collider(provider)
    , m_rateLimiter(limitInterval)
    , m_currentPosition{}
{
}

void PlayerMovementController::Teleport(const Client::Core::MapCoords& coords) noexcept {
    m_currentPosition = coords;
    m_rateLimiter.Reset();
}

EterBase::Result<std::optional<std::vector<uint8_t>>, Client::Core::CommandError>
PlayerMovementController::UpdateMovement(const Client::Core::MapCoords& nextCoords,
                                         float currentRotation,
                                         uint8_t moveType,
                                         uint32_t currentClientTime,
                                         MovementRateLimiter::TimePoint currentTime,
                                         bool forceSend)
{
    auto moveResult = m_collider.CalculateMovement(m_currentPosition, nextCoords);
    if (!moveResult.has_value()) {
        return std::unexpected(moveResult.error());
    }

    m_currentPosition = moveResult.value();

    if (m_rateLimiter.ShouldSendMovementPacket(currentTime, forceSend)) {
        Client::Core::MoveCommand cmd{};
        cmd.destination = m_currentPosition;
        cmd.rotation = currentRotation;
        cmd.moveType = moveType;
        cmd.clientTimestamp = currentClientTime;

        auto packetResult = Client::Network::MovementCommandEncoder::Encode(cmd);
        if (packetResult.has_value()) {
            return std::make_optional(std::move(packetResult.value()));
        }
        
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    return std::nullopt;
}

} // namespace Client::Gameplay
