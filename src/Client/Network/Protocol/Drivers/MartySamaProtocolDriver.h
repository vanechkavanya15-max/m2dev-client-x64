#pragma once

#include "../IServerProtocolDriver.h"

namespace Network::Protocol::Drivers
{
    /**
     * @brief Dedykowany sterownik protokolu serwerowego dla standardu MartySama (Ymir mainline).
     * 
     * CHARAKTERYSTYKA ARCHITEKTURY:
     * - Ramkowanie 1-bajtowe (HEADER_GC_*) z obsluga stalych i dynamicznych pakietow.
     * - Pakiety dynamiczne (CHAT, SCRIPT/QUEST, WHISPER itp.) zawieraja 2-bajtowy naglowek dlugosci bezposrednio po opkodzie.
     * - Bezposrednie kodowanie komend domenowych: Attack (8B), Move (16B), ItemUse (4B), Target (5B).
     * - Mapowanie pakietow przychodzacych na zdarzenia domenowe IGameEventSink oraz publikacja na EventBus.
     */
    class MartySamaProtocolDriver final : public IServerProtocolDriver
    {
    public:
        MartySamaProtocolDriver() noexcept = default;
        ~MartySamaProtocolDriver() override = default;

        [[nodiscard]] std::string_view GetDriverName() const noexcept override
        {
            return "martysama";
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

        using IServerProtocolDriver::EncodeItemUse;
        using IServerProtocolDriver::EncodeItemPickUp;
        using IServerProtocolDriver::EncodeTarget;

        bool DispatchInbound(
            uint16_t unifiedOpcode,
            std::span<const uint8_t> payload,
            UserInterface::Contracts::IGameEventSink* pSink) override;
    };
}
