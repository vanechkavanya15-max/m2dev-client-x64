#pragma once

#include <cstdint>
#include <span>
#include <unordered_map>
#include <functional>
#include "../../EterBase/Result.h"

namespace Network
{
    /**
     * @brief Sygnatura handlera pakietu w architekturze Zero-Alloc C++20 (kompatybilna).
     */
    using PacketHandlerFn = bool (*)(std::span<const uint8_t>);

    /**
     * @brief Nowoczesna sygnatura handlera C++23 zwracajaca dokladny PacketResult.
     */
    using ModernPacketHandlerFn = EterBase::PacketResult<void> (*)(std::span<const uint8_t>);

    /**
     * @brief Nowoczesny dispatcher pakietow gry Metin2 x64 (Standard 2026).
     * 
     * Zastepuje monolityczny switch-case w PythonNetworkStreamPhaseGame.cpp
     * szybka tablica dyspozytorska z opcodami uint16_t.
     */
    class PacketDispatcher
    {
    public:
        static PacketDispatcher& Instance();

        /**
         * @brief Rejestruje handler dla danego 16-bitowego naglowka pakietu GC.
         */
        void RegisterHandler(uint16_t opcode, PacketHandlerFn handler);

        /**
         * @brief Rejestruje nowoczesny handler C++23 zwracajacy PacketResult.
         */
        void RegisterModernHandler(uint16_t opcode, ModernPacketHandlerFn handler);

        /**
         * @brief Dysponuje surowy bufor pakietu do zarejestrowanego handlera domeny.
         * @param opcode Naglowek pakietu (np. GC::DEAD).
         * @param payload Bufor z zawartoscia pakietu.
         * @return true jesli pakiet zostal pomyslnie przetworzony.
         */
        bool Dispatch(uint16_t opcode, std::span<const uint8_t> payload);

        /**
         * @brief Dysponuje pakiet zwracajac PacketResult z kodem bledu przy niepowodzeniu.
         */
        EterBase::PacketResult<void> DispatchModern(uint16_t opcode, std::span<const uint8_t> payload);

        /**
         * @brief Rejestruje komplet domyslnych handlerow Fali 1.
         */
        void RegisterDefaultHandlers();

        /**
         * @brief Sprawdza czy dany opcode posiada zarejestrowany handler.
         */
        bool HasHandler(uint16_t opcode) const;

    private:
        PacketDispatcher() = default;
        ~PacketDispatcher() = default;

        std::unordered_map<uint16_t, PacketHandlerFn> m_handlers;
        std::unordered_map<uint16_t, ModernPacketHandlerFn> m_modernHandlers;
    };
}
