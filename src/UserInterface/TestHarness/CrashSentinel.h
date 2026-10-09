#pragma once

#include <windows.h>
#include <string>
#include <string_view>
#include <mutex>
#include <atomic>
#include <cstdint>

namespace UserInterface::TestHarness
{
    /**
     * @brief Wbudowany sentinel awarii C++23 dla klienta gry Metin2 x64.
     * 
     * Rejestruje wektorowy handler wyjatkow (AddVectoredExceptionHandler),
     * przechwytuje EXCEPTION_ACCESS_VIOLATION (0xC0000005) oraz inne wyjatki krytyczne,
     * bezpiecznie rozwija stos wywolan za pomoca DbgHelp / CaptureStackBackTrace z symboli PDB
     * i zapisuje szczegolowy raport w formacie JSON do bufora sentinelowego oraz na dysk.
     */
    class CrashSentinel
    {
    public:
        static CrashSentinel& Instance() noexcept;

        CrashSentinel() = default;
        ~CrashSentinel();

        CrashSentinel(const CrashSentinel&) = delete;
        CrashSentinel& operator=(const CrashSentinel&) = delete;
        CrashSentinel(CrashSentinel&&) = delete;
        CrashSentinel& operator=(CrashSentinel&&) = delete;

        /**
         * @brief Inicjalizuje sentinel wyjatkow i biblioteke DbgHelp.
         * @param reportPath Sciezka do pliku raportu JSON (domyslnie "crash_sentinel.json").
         * @return true jesli rejestracja powiodla sie.
         */
        bool Initialize(std::string_view reportPath = "crash_sentinel.json");

        /**
         * @brief Wyrejestrowuje handler wyjatkow i zwalnia zasoby DbgHelp.
         */
        void Shutdown();

        /**
         * @brief Sprawdza czy wystapil krytyczny wyjatek.
         */
        [[nodiscard]] bool HasCrashed() const noexcept { return m_hasCrashed.load(std::memory_order_relaxed); }

        /**
         * @brief Zwraca bufor ostatniego raportu o awarii w formacie JSON.
         */
        [[nodiscard]] std::string GetLastCrashJson() const;

        /**
         * @brief Ustawia sciezke docelowa pliku raportu awarii.
         */
        void SetReportPath(std::string_view reportPath);

        /**
         * @brief Statyczny wektorowy handler wyjatkow dla Windows API.
         */
        static LONG WINAPI VectoredExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo);

    private:
        LONG HandleException(PEXCEPTION_POINTERS pExceptionInfo);
        std::string BuildCrashJson(PEXCEPTION_POINTERS pExceptionInfo);
        void WriteReportToFile(const std::string& jsonContent);

    private:
        PVOID m_pHandlerHandle{nullptr};
        std::atomic<bool> m_isInitialized{false};
        std::atomic<bool> m_hasCrashed{false};
        std::atomic<bool> m_isHandlingCrash{false};
        std::string m_reportPath{"crash_sentinel.json"};
        mutable std::mutex m_mutex;
        std::string m_lastCrashJson;
    };
}
