#pragma once

#include "../IServerProtocolDriver.h"

namespace Network::Protocol::Drivers
{
    /**
     * @brief Dedykowany sterownik protokolu serwerowego dla platformy Beavium.
     * 
     * CHARAKTERYSTYKA:
     * - Ramkowanie 1-bajtowe oparte o tabele beavium_gc_table.inl (stale i dynamiczne)
     * - Specyficzne formaty pakietow: ProxyPacketCGAttack (8B), TPacketCGMoveBeavium (29B z mikrostopniami)
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

        [[nodiscard]] bool IsHandshakeOpcode(uint16_t opcode) const noexcept override;

        bool HandleHandshakePacket(
            uint16_t header,
            std::span<const uint8_t> payload,
            class CNetworkStream& stream) override;

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeLogin(
            const std::string& username,
            uint32_t loginKey) const override;

        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeAuthLogin(
            const std::string& username,
            const std::string& password,
            const std::string& pin = "") const override;

        using IServerProtocolDriver::EncodeItemUse;
        using IServerProtocolDriver::EncodeItemPickUp;
        using IServerProtocolDriver::EncodeTarget;

        bool DispatchInbound(
            uint16_t unifiedOpcode,
            std::span<const uint8_t> payload,
            UserInterface::Contracts::IGameEventSink* pSink) override;
    };
}
