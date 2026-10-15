#pragma once

#include <cstdint>
#include <atomic>
#include <string_view>

namespace UserInterface::TestHarness
{
    /**
     * @brief Deterministyczny kontroler klatek i taktu logiki gry pod MCP i testy automatyczne.
     * 
     * Obsluguje flagi wiersza polecen:
     *   --test-mode: aktywuje deterministyczny zarzadca klatek
     *   --freeze-on-start: zatrzymuje uplyw czasu logiki od pierwszej klatki
     */
    class DeterministicTickController
    {
    public:
        static DeterministicTickController& Instance() noexcept;

        DeterministicTickController() noexcept = default;
        ~DeterministicTickController() = default;

        DeterministicTickController(const DeterministicTickController&) = delete;
        DeterministicTickController& operator=(const DeterministicTickController&) = delete;

        /**
         * @brief Skanuje parametry wiersza polecen w poszukiwaniu flag --test-mode i --freeze-on-start.
         */
        void InitFromCommandLine(const char* lpCmdLine) noexcept;

        [[nodiscard]] bool IsTestMode() const noexcept { return m_isTestMode.load(std::memory_order_relaxed); }
        [[nodiscard]] bool IsFrozen() const noexcept { return m_isFrozen.load(std::memory_order_relaxed); }

        void SetTestMode(bool bMode) noexcept { m_isTestMode.store(bMode, std::memory_order_relaxed); }
        void SetFrozen(bool bFrozen) noexcept { m_isFrozen.store(bFrozen, std::memory_order_relaxed); }

        /**
         * @brief Zleca wykonanie dokladnie zadanej liczby klatek deterministycznych.
         * @param count Liczba klatek logiki do przepchniecia.
         * @param deltaTime Krok czasowy klatki w sekundach (domyslnie 1/60 s).
         */
        void Step(uint32_t count = 1, float deltaTime = 1.0f / 60.0f) noexcept;

        /**
         * @brief Pobiera i zeruje licznik oczekujacych krokow deterministycznych.
         */
        [[nodiscard]] uint32_t ConsumePendingSteps() noexcept;

        [[nodiscard]] float GetStepDeltaTime() const noexcept { return m_stepDeltaTime.load(std::memory_order_relaxed); }

        void IncrementTickCount() noexcept { m_currentTick.fetch_add(1, std::memory_order_relaxed); }
        [[nodiscard]] uint64_t GetCurrentTick() const noexcept { return m_currentTick.load(std::memory_order_relaxed); }

        void Reset() noexcept;

    private:
        std::atomic<bool> m_isTestMode{false};
        std::atomic<bool> m_isFrozen{false};
        std::atomic<uint32_t> m_pendingSteps{0};
        std::atomic<float> m_stepDeltaTime{1.0f / 60.0f};
        std::atomic<uint64_t> m_currentTick{0};
    };
}
