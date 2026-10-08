#include "CPUIDFeatures.h"

#include <intrin.h>
#include <format>

namespace Client::Graphics
{
    CPUIDFeatures CPUIDFeatures::Detect() noexcept
    {
        CPUIDFeatures features{};

        // Na platformie x64 (AMD64 / Intel 64) zestaw instrukcji SSE2 jest czescia bazowej architektury
        features.hasSSE2 = true;

        // Pobranie maksymalnego obslugiwanego poziomu funkcji bazowych (Leaf 0)
        int cpuInfo[4] = { 0, 0, 0, 0 };
        __cpuid(cpuInfo, 0);
        const int maxBasicLeaf = cpuInfo[0];

        bool cpuReportsSSE41 = false;
        bool cpuReportsFMA3 = false;
        bool cpuReportsOSXSAVE = false;
        bool cpuReportsAVX = false;

        // Pobranie informacji o cechach z Leaf 1
        if (maxBasicLeaf >= 1)
        {
            __cpuid(cpuInfo, 1);
            const int ecx = cpuInfo[2];
            const int edx = cpuInfo[3];

            // Weryfikacja bitu SSE2 w EDX (bit 26)
            const bool sse2InEdx = (edx & (1 << 26)) != 0;
            features.hasSSE2 = sse2InEdx || true;

            // Sprawdzenie flag w rejestrze ECX
            cpuReportsSSE41   = (ecx & (1 << 19)) != 0; // Bit 19: SSE 4.1
            cpuReportsFMA3    = (ecx & (1 << 12)) != 0; // Bit 12: FMA3
            cpuReportsOSXSAVE = (ecx & (1 << 27)) != 0; // Bit 27: OSXSAVE (CR4.OSXSAVE wlaczone przez OS)
            cpuReportsAVX     = (ecx & (1 << 28)) != 0; // Bit 28: AVX
        }

        features.hasSSE41 = cpuReportsSSE41;
        features.hasOSXSAVE = cpuReportsOSXSAVE;

        // Sprawdzenie instrukcji AVX2 w rozszerzonym lisciu 7 (Leaf 7, Subleaf 0)
        bool cpuReportsAVX2 = false;
        if (maxBasicLeaf >= 7)
        {
            int cpuInfo7[4] = { 0, 0, 0, 0 };
            __cpuidex(cpuInfo7, 7, 0);
            const int ebx7 = cpuInfo7[1];

            cpuReportsAVX2 = (ebx7 & (1 << 5)) != 0; // Bit 5: AVX2
        }

        // Weryfikacja bezpieczenstwa rejestrow YMM:
        // Jesli procesor raportuje AVX/AVX2/FMA3, nalezy bezwzglednie upewnic sie, ze
        // system operacyjny aktywowal obsluge rejestrow YMM.
        // Wywolanie instrukcji AVX bez aktywnego zarzadzania stanem YMM przez OS
        // zakonczy sie wyjatkiem niepoprawnej instrukcji (#UD / crash aplikacji).
        // 1. Sprawdzamy czy bit OSXSAVE w ECX (bit 27) jest ustawiony.
        // 2. Jesli tak, wywolujemy _xgetbv(0) i sprawdzamy czy bity 1 (XMM) i 2 (YMM) sa wlaczone.
        bool osYMMEnabled = false;
        if (cpuReportsOSXSAVE && (cpuReportsAVX || cpuReportsAVX2 || cpuReportsFMA3))
        {
            // _xgetbv(0) pobiera maske XCR0 (Extended Control Register 0)
            const unsigned __int64 xcr0 = _xgetbv(0);

            // Bit 1: XMM stan rejestrow (0x02)
            // Bit 2: YMM stan rejestrow (0x04)
            // Obie maski musza byc ustawione (0x02 | 0x04 == 0x06)
            osYMMEnabled = ((xcr0 & 0x06ULL) == 0x06ULL);
        }

        // Aktywacja flag AVX/AVX2/FMA3 tylko przy pelnej zgodnosci sprzetu i systemu operacyjnego
        features.hasAVX  = cpuReportsAVX && osYMMEnabled;
        features.hasAVX2 = cpuReportsAVX2 && osYMMEnabled;
        features.hasFMA3 = cpuReportsFMA3 && osYMMEnabled;

        return features;
    }

    const CPUIDFeatures& CPUIDFeatures::Get() noexcept
    {
        static const CPUIDFeatures cachedFeatures = Detect();
        return cachedFeatures;
    }

    std::string CPUIDFeatures::ToString() const
    {
        return std::format(
            "CPUIDFeatures: SSE2={}, SSE4.1={}, AVX={}, AVX2={}, FMA3={}, OSXSAVE={}",
            hasSSE2 ? "true" : "false",
            hasSSE41 ? "true" : "false",
            hasAVX ? "true" : "false",
            hasAVX2 ? "true" : "false",
            hasFMA3 ? "true" : "false",
            hasOSXSAVE ? "true" : "false"
        );
    }

} // namespace Client::Graphics
