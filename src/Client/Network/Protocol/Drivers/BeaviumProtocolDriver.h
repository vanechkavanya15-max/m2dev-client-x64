#pragma once

#include "../IServerProtocolDriver.h"

namespace Network::Protocol::Drivers
{
    /**
     * @brief Dedykowany sterownik protokolu serwerowego dla platformy Beavium.
     * 
     * CHARAKTERYSTYKA:
     * - Ramkowanie 1-bajtowe oparte o tabele beavium_gc_table.inl (stale i dynamiczne)
     * - Specyficzne formaty pakietow: ProxyPacketCGAttack (8B), TPacketCGMoveBeavium (24B z mikrostopniami)
     * - Dyspozycja bezposrednia do IGameEventSink oraz publikacja w EventBus
     */
    class BeaviumProtocolDriver final : public IServerProtocolDriver
    {
    public:
        BeaviumProtocolDriver() noexcept = default;
        ~BeaviumProtocolDriver() override = default;

        [[nodiscard]] std::string_view GetDriverName() const noexcept override
        {
            return "beavium";
        }

        [[nodiscard]] std::optional<FrameHeaderInfo> InspectFrame(
            std::span<const uint8_t> buffer) const noexcept override;

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeAttack(
            const Domain::AttackCommand& cmd) const override;

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeMove(
            const Domain::MoveCommand& cmd) const override;

        bool DispatchInbound(
            uint16_t unifiedOpcode,
            std::span<const uint8_t> payload,
            UserInterface::Contracts::IGameEventSink* pSink) override;
    };
}
