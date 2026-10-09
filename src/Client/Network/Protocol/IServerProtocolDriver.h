#pragma once

#include <cstdint>
#include <string_view>
#include <optional>
#include <span>
#include <vector>
#include "EterBase/Result.h"
#include "../Domain/CombatCommands.h"
#include "../Domain/MovementCommands.h"

namespace UserInterface::Contracts
{
    class IGameEventSink;
}

namespace Network::Protocol
{
    /**
     * @brief Informacje o zbadanej ramce pakietu w strumieniu bufora TCP.
     */
    struct FrameHeaderInfo
    {
        uint16_t unifiedOpcode{0}; ///< Zunifikowany naglowek wewnetrzny
        uint32_t packetLength{0};  ///< Calkowity rozmiar pakietu do odebrania z bufora TCP
        uint32_t headerSize{0};    ///< Rozmiar naglowka (1B dla Beavium/Legacy, 4B dla x64)
    };

    /**
     * @brief Czysty interfejs sterownika protokolu serwera w architekturze UPM.
     */
    class IServerProtocolDriver
    {
    public:
        virtual ~IServerProtocolDriver() = default;

        /**
         * @brief Zwraca unikalna nazwe identyfikacyjna sterownika (np. "standard_x64", "beavium").
         */
        [[nodiscard]] virtual std::string_view GetDriverName() const noexcept = 0;

        /**
         * @brief Bada bufor TCP i zwraca naglowek oraz dlugosc pakietu, lub nullopt jesli potrzeba wiecej danych.
         * @param buffer Widok na poczatek bufora odbiorczego TCP.
         */
        [[nodiscard]] virtual std::optional<FrameHeaderInfo> InspectFrame(
            std::span<const uint8_t> buffer) const noexcept = 0;

        /**
         * @brief Koduje czysta komende ataku do formatu binarnego serwera docelowego.
         * @param cmd Parametry komendy ataku.
         * @return Wektor zakodowanych bajtow lub kod bledu PacketResult.
         */
        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeAttack(
            const Domain::AttackCommand& cmd) const = 0;

        /**
         * @brief Koduje czysta komende ruchu do formatu binarnego serwera docelowego.
         * @param cmd Parametry komendy ruchu.
         * @return Wektor zakodowanych bajtow lub kod bledu PacketResult.
         */
        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeMove(
            const Domain::MoveCommand& cmd) const = 0;

        /**
         * @brief Dyspozycja zdeserializowanego pakietu przychodzacego do odbiornika zdarzen.
         * @param unifiedOpcode Zunifikowany lub surowy kod opkodu.
         * @param payload Calkowity pakiet lub payload pakietu.
         * @param pSink Opcjonalny wskaznik na odbiornik kontraktowy IGameEventSink.
         * @return true jesli pakiet zostal pomyslnie obsluzony, false w przeciwnym razie.
         */
        virtual bool DispatchInbound(
            uint16_t unifiedOpcode,
            std::span<const uint8_t> payload,
            UserInterface::Contracts::IGameEventSink* pSink) = 0;
    };
}
