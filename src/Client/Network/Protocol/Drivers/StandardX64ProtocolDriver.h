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

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeItemUse(
            uint16_t cell, uint8_t windowType) const override;

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeTarget(
            uint32_t targetVid) const override;

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeItemMove(
            const Domain::ItemMoveCommand& cmd) const override;

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeItemDrop(
            const Domain::ItemDropCommand& cmd) const override;

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeItemPickUp(
            const Domain::ItemPickUpCommand& cmd) const override;

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeSelectEmpire(
            const Domain::EmpireSelectCommand& cmd) const override;

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeSelectCharacter(
            const Domain::CharacterSelectCommand& cmd) const override;

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeCreateCharacter(
            const Domain::CharacterCreateCommand& cmd) const override;

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeDestroyCharacter(
            const Domain::CharacterDestroyCommand& cmd) const override;

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeEnterGame() const override;

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeLogin(
            const std::string& username,
            uint32_t loginKey) const override;

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeAuthLogin(
            const std::string& username,
            const std::string& password,
            const std::string& pin = "") const override;

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeClientVersion(
            const std::string& filename) const override;

        [[nodiscard]] bool IsHandshakeOpcode(uint16_t opcode) const noexcept override;

        bool HandleHandshakePacket(
            uint16_t header,
            std::span<const uint8_t> payload,
            class CNetworkStream& stream) override;

        using IServerProtocolDriver::EncodeItemUse;
        using IServerProtocolDriver::EncodeItemPickUp;
        using IServerProtocolDriver::EncodeTarget;

        bool DispatchInbound(
            uint16_t unifiedOpcode,
            std::span<const uint8_t> payload,
            UserInterface::Contracts::IGameEventSink* pSink) override;
    };
}
