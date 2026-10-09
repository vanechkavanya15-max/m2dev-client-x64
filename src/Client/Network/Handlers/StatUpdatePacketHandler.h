#pragma once

#include <cstdint>
#include <span>
#include "EterBase/Result.h"
#include "Client/Network/ModernPacketDispatcher.h"
#include "Client/Gameplay/IPlayerStatsService.h"

namespace Client::Network::Handlers {

    class PointsPacketHandler final : public IPacketHandler {
    public:
        explicit PointsPacketHandler(Client::Gameplay::IPlayerStatsService* statsService) noexcept;
        ~PointsPacketHandler() override = default;

        [[nodiscard]] EterBase::PacketResult<void> Handle(std::span<const uint8_t> payload) override;
        [[nodiscard]] uint16_t GetExpectedSize() const noexcept override;
        [[nodiscard]] bool IsDynamicSize() const noexcept override;

    private:
        Client::Gameplay::IPlayerStatsService* m_statsService;
    };

    class PointChangePacketHandler final : public IPacketHandler {
    public:
        explicit PointChangePacketHandler(Client::Gameplay::IPlayerStatsService* statsService) noexcept;
        ~PointChangePacketHandler() override = default;

        [[nodiscard]] EterBase::PacketResult<void> Handle(std::span<const uint8_t> payload) override;
        [[nodiscard]] uint16_t GetExpectedSize() const noexcept override;
        [[nodiscard]] bool IsDynamicSize() const noexcept override;

    private:
        Client::Gameplay::IPlayerStatsService* m_statsService;
    };

} // namespace Client::Network::Handlers
