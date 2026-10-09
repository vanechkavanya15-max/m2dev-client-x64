#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <atomic>
#include <cstring>

#ifndef DWORD
typedef unsigned long DWORD;
#endif

namespace UserInterface::TestHarness
{
    /**
     * @brief Sterownik symulacji lokalnego swiata gry bez polaczenia sieciowego (--mock-world).
     * 
     * Inicjalizuje glownego gracza w ActorRegistry, konfiguruje statystyki i kontrolery
     * oraz bezposrednio przelacza silnik do PhaseGame bez koniecznosci zestawiania gniazda TCP.
     */
    class MockWorldDriver
    {
    public:
        static MockWorldDriver& Instance() noexcept
        {
            static MockWorldDriver s_instance;
            return s_instance;
        }

        MockWorldDriver() noexcept = default;
        ~MockWorldDriver() = default;

        MockWorldDriver(const MockWorldDriver&) = delete;
        MockWorldDriver& operator=(const MockWorldDriver&) = delete;

        /**
         * @brief Skanuje parametry wiersza polecen w poszukiwaniu flagi --mock-world.
         */
        void InitFromCommandLine(const char* lpCmdLine) noexcept
        {
            if (!lpCmdLine)
                return;

            if (std::strstr(lpCmdLine, "--mock-world") != nullptr)
            {
                m_isMockWorldEnabled.store(true, std::memory_order_relaxed);
            }
        }

        [[nodiscard]] bool IsMockWorldEnabled() const noexcept { return m_isMockWorldEnabled.load(std::memory_order_relaxed); }
        [[nodiscard]] bool HasEntered() const noexcept { return m_hasEntered.load(std::memory_order_relaxed); }

        /**
         * @brief Inicjalizuje instancje postaci gracza, rejestruje w ActorRegistry i przelacza w PhaseGame.
         * @param dwMainVID VirtualID dla postaci gracza (domyslnie 10001).
         * @param dwRace Numer rasy/klasy (domyslnie 0 - Wojownik meski).
         * @param name Nazwa postaci (domyslnie "AI_Agent_Mock").
         * @param posX Poczatkowa pozycja X w jednostkach gry.
         * @param posY Poczatkowa pozycja Y w jednostkach gry.
         * @return true jesli faza i gracz zostali pomyslnie zainicjalizowani.
         */
        bool EnterMockWorld(
            DWORD dwMainVID = 10001,
            DWORD dwRace = 0,
            const std::string& name = "AI_Agent_Mock",
            long posX = 10000,
            long posY = 10000);

        void Reset() noexcept
        {
            m_isMockWorldEnabled.store(false, std::memory_order_relaxed);
            m_hasEntered.store(false, std::memory_order_relaxed);
            m_dwMockVID = 10001;
        }

    private:
        std::atomic<bool> m_isMockWorldEnabled{false};
        std::atomic<bool> m_hasEntered{false};
        DWORD m_dwMockVID{10001};
    };
}
