#pragma once

#include "ItemGroundAddHandler.h"
#include "../../Network/ModernPacketDispatcher.h"

namespace Client::Network
{
    class ItemGroundAddPacketHandler final : public IPacketHandler
    {
    public:
        ItemGroundAddPacketHandler() noexcept = default;
        ~ItemGroundAddPacketHandler() override = default;

        [[nodiscard]] EterBase::PacketResult<void> Handle(std::span<const uint8_t> payload) override
        {
            return Client::Network::Handlers::ProcessItemGroundAdd(payload);
        }
        
        [[nodiscard]] constexpr uint16_t GetExpectedSize() const noexcept override
        {
            return sizeof(Client::Network::Handlers::PacketItemGroundAdd);
        }
        
        [[nodiscard]] constexpr bool IsDynamicSize() const noexcept override
        {
            return false;
        }
    };
}
