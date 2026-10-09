#pragma once

#include "../IServerProtocolDriver.h"

namespace Network::Protocol::Drivers
{
    /**
     * @brief Dedykowany sterownik protokolu serwerowego dla standardu Metin2 x64 2026.
     * 
     * CHARAKTERYSTYKA:
     * - Unifikacja ramkowania 4-bajtowego: [Header:2][Length:2][Payload...]
     * - Standardowe struktury pakietowe: TPacketCGAttack (11B), TPacketCGMove (19B)
     * - Dyspozycja do PhaseGamePacketDispatcher
     */
    class StandardX64ProtocolDriver final : public IServerProtocolDriver
    {
    public:
        StandardX64ProtocolDriver() noexcept = default;
        ~StandardX64ProtocolDriver() override = default;

        [[nodiscard]] std::string_view GetDriverName() const noexcept override
        {
            return "standard_x64";
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
