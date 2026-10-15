#pragma once

#include <cstdint>
#include <string>

namespace Client::Graphics
{
    /**
     * @brief Struktura przechowujaca flagi dostepnosci instrukcji wektorowych i cech procesora.
     */
    struct CPUIDInfo
    {
        bool hasSSE2{ true };    // Architektura x64 gwarantuje SSE2 jako zestaw bazowy
        bool hasSSE41{ false };   // SSE 4.1
        bool hasAVX{ false };     // Advanced Vector Extensions (rejestry YMM 256-bit)
        bool hasAVX2{ false };    // Advanced Vector Extensions 2
        bool hasFMA3{ false };    // Fused Multiply-Add 3-operandy
        bool hasOSXSAVE{ false }; // Flaga OSXSAVE (wsparcie OS dla instrukcji XSAVE/XRSTOR)
    };

    /**
     * @brief Modul detekcji rozszerzen procesora (CPUIDFeatures).
     * Weryfikuje wsparcie sprzetowe oraz bezpieczenstwo rejestrow YMM w systemie operacyjnym.
     */
    struct CPUIDFeatures : public CPUIDInfo
    {
        /**
         * @brief Zwraca zcache'owana instancje singletona ze zdetektowanymi cechami procesora.
         */
        [[nodiscard]] static const CPUIDFeatures& Get() noexcept;

        /**
         * @brief Wykonuje bezposrednia detekcje cech procesora za pomoca instrukcji CPUID oraz _xgetbv.
         */
        [[nodiscard]] static CPUIDFeatures Detect() noexcept;

        // Metody pomocnicze dostepu do flag (const noexcept)
        [[nodiscard]] constexpr bool HasSSE2() const noexcept { return hasSSE2; }
        [[nodiscard]] constexpr bool HasSSE41() const noexcept { return hasSSE41; }
        [[nodiscard]] constexpr bool HasAVX() const noexcept { return hasAVX; }
        [[nodiscard]] constexpr bool HasAVX2() const noexcept { return hasAVX2; }
        [[nodiscard]] constexpr bool HasFMA3() const noexcept { return hasFMA3; }
        [[nodiscard]] constexpr bool HasOSXSAVE() const noexcept { return hasOSXSAVE; }

        /**
         * @brief Zwraca czytelny opis tekstowy zdetektowanych instrukcji CPU.
         */
        [[nodiscard]] std::string ToString() const;
    };

} // namespace Client::Graphics

namespace Graphics
{
    using CPUIDInfo = Client::Graphics::CPUIDInfo;
    using CPUIDFeatures = Client::Graphics::CPUIDFeatures;
}

// Globalne aliasy dla wygody i pelnej kompatybilnosci wstecznej
using Client::Graphics::CPUIDInfo;
using Client::Graphics::CPUIDFeatures;
