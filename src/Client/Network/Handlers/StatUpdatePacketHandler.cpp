#include "StatUpdatePacketHandler.h"
#include "Client/Network/Protocol/Protocol.h"

namespace Client::Network::Handlers {

    PointsPacketHandler::PointsPacketHandler(Client::Gameplay::IPlayerStatsService* statsService) noexcept
        : m_statsService(statsService)
    {
    }

    EterBase::PacketResult<void> PointsPacketHandler::Handle(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(TPacketGCPoints))
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);

        if (!m_statsService)
            return EterBase::MakeError(EterBase::PacketError::SessionClosed); // Brak instancji serwisu

        const auto* packet = reinterpret_cast<const TPacketGCPoints*>(payload.data());
        
        for (uint32_t i = 0; i < POINT_MAX_NUM; ++i) {
            m_statsService->SetPoint(i, packet->points[i]);
        }

        return {};
    }

    uint16_t PointsPacketHandler::GetExpectedSize() const noexcept
    {
        return sizeof(TPacketGCPoints);
    }

    bool PointsPacketHandler::IsDynamicSize() const noexcept
    {
        return false;
    }


    PointChangePacketHandler::PointChangePacketHandler(Client::Gameplay::IPlayerStatsService* statsService) noexcept
        : m_statsService(statsService)
    {
    }

    EterBase::PacketResult<void> PointChangePacketHandler::Handle(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(TPacketGCPointChange))
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);

        if (!m_statsService)
            return EterBase::MakeError(EterBase::PacketError::SessionClosed); // Brak instancji serwisu

        const auto* packet = reinterpret_cast<const TPacketGCPointChange*>(payload.data());

        if (packet->Type >= POINT_MAX_NUM)
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);

        m_statsService->SetPoint(packet->Type, packet->value);

        return {};
    }

    uint16_t PointChangePacketHandler::GetExpectedSize() const noexcept
    {
        return sizeof(TPacketGCPointChange);
    }

    bool PointChangePacketHandler::IsDynamicSize() const noexcept
    {
        return false;
    }

} // namespace Client::Network::Handlers
