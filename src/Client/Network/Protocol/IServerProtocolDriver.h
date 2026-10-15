#pragma once

#include <cstdint>
#include <string_view>
#include <optional>
#include <span>
#include <vector>
#include "EterBase/Result.h"
#include "Protocol_Common.h"
#include "../Domain/CombatCommands.h"
#include "../Domain/MovementCommands.h"
#include "../Domain/ItemCommands.h"
#include "../Domain/SelectCommands.h"

class CNetworkStream;

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
         * @brief Koduje komende uzycia przedmiotu do formatu binarnego serwera docelowego.
         * @param cell Indeks komorki/slotu w oknie ekwipunku.
         * @param windowType Typ okna (np. INVENTORY, EQUIPMENT).
         * @return Wektor zakodowanych bajtow lub kod bledu PacketResult.
         */
        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeItemUse(
            uint16_t cell, uint8_t windowType) const = 0;

        /**
         * @brief Przeciazenie kodowania uzycia przedmiotu na podstawie struktury domenowej.
         */
        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeItemUse(
            const Domain::ItemUseCommand& cmd) const
        {
            return EncodeItemUse(cmd.cell, cmd.window);
        }

        /**
         * @brief Koduje komende przemieszczenia przedmiotu w ekwipunku.
         */
        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeItemMove(
            const Domain::ItemMoveCommand& /*cmd*/) const
        {
            return std::unexpected(EterBase::PacketError::InvalidHeader);
        }

        /**
         * @brief Koduje komende wyrzucenia przedmiotu lub zlota na ziemie.
         */
        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeItemDrop(
            const Domain::ItemDropCommand& /*cmd*/) const
        {
            return std::unexpected(EterBase::PacketError::InvalidHeader);
        }

        /**
         * @brief Koduje komende podniesienia przedmiotu z ziemi.
         */
        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeItemPickUp(
            const Domain::ItemPickUpCommand& /*cmd*/) const
        {
            return std::unexpected(EterBase::PacketError::InvalidHeader);
        }

        /**
         * @brief Przeciazenie podniesienia przedmiotu po VID.
         */
        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeItemPickUp(uint32_t vid) const
        {
            return EncodeItemPickUp(Domain::ItemPickUpCommand{ .vid = vid });
        }

        /**
         * @brief Koduje komende wyboru krolestwa (Empire).
         */
        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeSelectEmpire(
            const Domain::EmpireSelectCommand& /*cmd*/) const
        {
            return std::unexpected(EterBase::PacketError::InvalidHeader);
        }

        /**
         * @brief Koduje komende wyboru postaci ze slotu.
         */
        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeSelectCharacter(
            const Domain::CharacterSelectCommand& /*cmd*/) const
        {
            return std::unexpected(EterBase::PacketError::InvalidHeader);
        }

        /**
         * @brief Koduje komende tworzenia nowej postaci.
         */
        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeCreateCharacter(
            const Domain::CharacterCreateCommand& /*cmd*/) const
        {
            return std::unexpected(EterBase::PacketError::InvalidHeader);
        }

        /**
         * @brief Koduje komende usuniecia postaci kodem prywatnym (PIN).
         */
        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeDestroyCharacter(
            const Domain::CharacterDestroyCommand& /*cmd*/) const
        {
            return std::unexpected(EterBase::PacketError::InvalidHeader);
        }

        /**
         * @brief Koduje komende wejscia do gry (EnterGame / EnterFrontGame) do formatu binarnego serwera docelowego.
         * @return Wektor zakodowanych bajtow lub kod bledu PacketResult.
         */
        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeEnterGame() const
        {
            return std::unexpected(EterBase::PacketError::InvalidHeader);
        }

        /**
         * @brief Koduje komende wyboru celu (target VID) do formatu binarnego serwera docelowego.
         * @param targetVid Wirtualny identyfikator celu.
         * @return Wektor zakodowanych bajtow lub kod bledu PacketResult.
         */
        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeTarget(
            uint32_t targetVid) const = 0;

        /**
         * @brief Przeciazenie kodowania wyboru celu na podstawie struktury domenowej.
         */
        [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeTarget(
            const Domain::TargetCommand& cmd) const
        {
            return EncodeTarget(cmd.targetVid);
        }

        /**
         * @brief Minimalny interwal w milisekundach miedzy kolejnymi atakami dla danego protokolu.
         */
        [[nodiscard]] virtual uint32_t GetMinAttackIntervalMs() const noexcept { return 150; }

        /**
         * @brief Minimalny interwal w milisekundach miedzy kolejnymi akcjami (np. uzyciem przedmiotu).
         */
        [[nodiscard]] virtual uint32_t GetMinActionIntervalMs() const noexcept { return 100; }

        /**
         * @brief Sprawdza czy dany kod operacji nalezy do procedury handshake/kluczy tego protokolu.
         */
        [[nodiscard]] virtual bool IsHandshakeOpcode(uint16_t /*opcode*/) const noexcept { return false; }

        /**
         * @brief Obsluguje pakiet handshake/wymiany kluczy bezposrednio w sterowniku.
         * @param header Naglowek pakietu.
         * @param payload Zawartosc calego pakietu.
         * @param stream Referencja do bazowego CNetworkStream.
         * @return true jesli pakiet zostal w pelni obsluzony przez sterownik.
         */
        virtual bool HandleHandshakePacket(
            uint16_t /*header*/,
            std::span<const uint8_t> /*payload*/,
            class CNetworkStream& /*stream*/)
        {
            return false;
        }

        /**
         * @brief Koduje pakiet logowania do serwera gry po udanej autoryzacji (Phase Login -> Select).
         * @param username Nazwa uzytkownika.
         * @param loginKey Klucz autoryzacji sesji.
         */
        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeLogin(
            const std::string& username,
            uint32_t loginKey) const
        {
            TPacketCGLogin2 pkt{};
            pkt.header = 0x6D; // CG::LOGIN2
            pkt.length = sizeof(pkt);
            pkt.login_key = loginKey;
            strncpy_s(pkt.name, sizeof(pkt.name), username.c_str(), _TRUNCATE);
            std::vector<uint8_t> out(sizeof(pkt));
            std::memcpy(out.data(), &pkt, sizeof(pkt));
            return out;
        }

        /**
         * @brief Koduje pakiet autoryzacji do serwera auth (Phase Handshake -> Auth).
         * @param username Nazwa uzytkownika.
         * @param password Haslo.
         * @param pin Opcjonalny kod PIN.
         */
        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeAuthLogin(
            const std::string& username,
            const std::string& password,
            const std::string& /*pin*/ = "") const
        {
            TPacketCGLogin3 pkt{};
            pkt.header = 0x6F; // CG::LOGIN3
            pkt.length = sizeof(pkt);
            strncpy_s(pkt.name, sizeof(pkt.name), username.c_str(), _TRUNCATE);
            strncpy_s(pkt.pwd, sizeof(pkt.pwd), password.c_str(), _TRUNCATE);
            std::vector<uint8_t> out(sizeof(pkt));
            std::memcpy(out.data(), &pkt, sizeof(pkt));
            return out;
        }

        /**
         * @brief Koduje pakiet raportu wersji klienta (ClientVersion).
         * @param filename Nazwa pliku wykonywalnego klienta.
         */
        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeClientVersion(
            const std::string& filename) const
        {
            TPacketCGClientVersion pkt{};
            pkt.header = 0x000D; // CG::CLIENT_VERSION
            pkt.length = sizeof(pkt);
            const char szDefName[] = "Metin2_Release.exe";
            const char szDefTimestamp[] = "1215955205";
            strncpy_s(pkt.filename, sizeof(pkt.filename), filename.empty() ? szDefName : filename.c_str(), _TRUNCATE);
            strncpy_s(pkt.timestamp, sizeof(pkt.timestamp), szDefTimestamp, _TRUNCATE);
            std::vector<uint8_t> out(sizeof(pkt));
            std::memcpy(out.data(), &pkt, sizeof(pkt));
            return out;
        }

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
