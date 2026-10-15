#pragma once

#include <cstdint>
#include <array>
#include <span>

namespace Luria
{
    struct LuriaDH2Keys
    {
        std::array<uint8_t, 256> cgPubData{}; // Klucz publiczny klienta wysylany w 0xFB (256 B)
        std::array<uint8_t, 16>  key1{};      // Klucz deszyfrowania RX (Blowfish CTR, 16 B)
        std::array<uint8_t, 16>  key2{};      // Klucz szyfrowania TX (Blowfish CTR, 16 B)
        std::array<uint8_t, 8>   iv1{};       // Wektor IV dla RX (8 B)
        std::array<uint8_t, 8>   iv2{};       // Wektor IV dla TX (8 B)
    };

    /**
     * @brief Negocjuje klucze Diffie-Hellman DH2 MODP 1024-bit dla serwera Luria.
     * Generuje klucze spelniajace warunek sumy kontrolnej: c1 == 11 && c2 == 11 (Blowfish CTR).
     * @param serverPubData 256 bajtow klucza publicznego serwera odebranego w pakiecie 0xFB.
     * @param outKeys Struktura wynikowa zawierajaca cgPubData, key1, key2, iv1, iv2.
     * @return true jesli wynegocjowano klucze pomyslnie, false w razie bledu.
     */
    bool NegotiateDH2Keys(std::span<const uint8_t> serverPubData, LuriaDH2Keys& outKeys);

    /**
     * @brief Weryfikacja spojnosci arytmetyki ModPow z wektorem referencyjnym Pythona.
     */
    bool TestModPowConsistency();

} // namespace Luria
