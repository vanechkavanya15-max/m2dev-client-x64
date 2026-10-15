#include "LuriaDH2.h"
#include <cstring>
#include <random>
#include <intrin.h>

namespace Luria
{
    namespace
    {
        struct BigInt1024
        {
            std::array<uint64_t, 16> w{}; // w[0] = najmniej znaczacy limb, w[15] = najbardziej znaczacy
        };

        struct BigInt2048
        {
            std::array<uint64_t, 32> w{};
        };

        uint64_t ReadUint64BE(const uint8_t* ptr) noexcept
        {
            return (static_cast<uint64_t>(ptr[0]) << 56) |
                   (static_cast<uint64_t>(ptr[1]) << 48) |
                   (static_cast<uint64_t>(ptr[2]) << 40) |
                   (static_cast<uint64_t>(ptr[3]) << 32) |
                   (static_cast<uint64_t>(ptr[4]) << 24) |
                   (static_cast<uint64_t>(ptr[5]) << 16) |
                   (static_cast<uint64_t>(ptr[6]) << 8)  |
                   static_cast<uint64_t>(ptr[7]);
        }

        void WriteUint64BE(uint8_t* ptr, uint64_t val) noexcept
        {
            ptr[0] = static_cast<uint8_t>((val >> 56) & 0xFF);
            ptr[1] = static_cast<uint8_t>((val >> 48) & 0xFF);
            ptr[2] = static_cast<uint8_t>((val >> 40) & 0xFF);
            ptr[3] = static_cast<uint8_t>((val >> 32) & 0xFF);
            ptr[4] = static_cast<uint8_t>((val >> 24) & 0xFF);
            ptr[5] = static_cast<uint8_t>((val >> 16) & 0xFF);
            ptr[6] = static_cast<uint8_t>((val >> 8) & 0xFF);
            ptr[7] = static_cast<uint8_t>(val & 0xFF);
        }

        BigInt1024 FromBytes128BE(const uint8_t* bytes) noexcept
        {
            BigInt1024 res{};
            for (size_t i = 0; i < 16; ++i)
            {
                res.w[i] = ReadUint64BE(bytes + 128 - 8 * (i + 1));
            }
            return res;
        }

        void ToBytes128BE(const BigInt1024& val, uint8_t* dest) noexcept
        {
            for (size_t i = 0; i < 16; ++i)
            {
                WriteUint64BE(dest + 128 - 8 * (i + 1), val.w[i]);
            }
        }

        // Stale kryptograficzne DH2 Lurii (MODP 1024-bit)
        // P (128 B, 1024-bit MODP Prime)
        alignas(16) const uint8_t P_BYTES[128] = {
            0xB1, 0x0B, 0x8F, 0x96, 0xA0, 0x80, 0xE0, 0x1D, 0xDE, 0x92, 0xDE, 0x5E, 0xAE, 0x5D, 0x54, 0xEC,
            0x52, 0xC9, 0x9F, 0xBC, 0xFB, 0x06, 0xA3, 0xC6, 0x9A, 0x6A, 0x9D, 0xCA, 0x52, 0xD2, 0x3B, 0x61,
            0x60, 0x73, 0xE2, 0x86, 0x75, 0xA2, 0x3D, 0x18, 0x98, 0x38, 0xEF, 0x1E, 0x2E, 0xE6, 0x52, 0xC0,
            0x13, 0xEC, 0xB4, 0xAE, 0xA9, 0x06, 0x11, 0x23, 0x24, 0x97, 0x5C, 0x3C, 0xD4, 0x9B, 0x83, 0xBF,
            0xAC, 0xCB, 0xDD, 0x7D, 0x90, 0xC4, 0xBD, 0x70, 0x98, 0x48, 0x8E, 0x9C, 0x21, 0x9A, 0x73, 0x72,
            0x4E, 0xFF, 0xD6, 0xFA, 0xE5, 0x64, 0x47, 0x38, 0xFA, 0xA3, 0x1A, 0x4F, 0xF5, 0x5B, 0xCC, 0xC0,
            0xA1, 0x51, 0xAF, 0x5F, 0x0D, 0xC8, 0xB4, 0xBD, 0x45, 0xBF, 0x37, 0xDF, 0x36, 0x5C, 0x1A, 0x65,
            0xE6, 0x8C, 0xFD, 0xA7, 0x6D, 0x4D, 0xA7, 0x08, 0xDF, 0x1F, 0xB2, 0xBC, 0x2E, 0x4A, 0x43, 0x71
        };

        // G (128 B, Generator)
        alignas(16) const uint8_t G_BYTES[128] = {
            0xA4, 0xD1, 0xCB, 0xD5, 0xC3, 0xFD, 0x34, 0x12, 0x67, 0x65, 0xA4, 0x42, 0xEF, 0xB9, 0x99, 0x05,
            0xF8, 0x10, 0x4D, 0xD2, 0x58, 0xAC, 0x50, 0x7F, 0xD6, 0x40, 0x6C, 0xFF, 0x14, 0x26, 0x6D, 0x31,
            0x26, 0x6F, 0xEA, 0x1E, 0x5C, 0x41, 0x56, 0x4B, 0x77, 0x7E, 0x69, 0x0F, 0x55, 0x04, 0xF2, 0x13,
            0x16, 0x02, 0x17, 0xB4, 0xB0, 0x1B, 0x88, 0x6A, 0x5E, 0x91, 0x54, 0x7F, 0x9E, 0x27, 0x49, 0xF4,
            0xD7, 0xFB, 0xD7, 0xD3, 0xB9, 0xA9, 0x2E, 0xE1, 0x90, 0x9D, 0x0D, 0x22, 0x63, 0xF8, 0x0A, 0x76,
            0xA6, 0xA2, 0x4C, 0x08, 0x7A, 0x09, 0x1F, 0x53, 0x1D, 0xBF, 0x0A, 0x01, 0x69, 0xB6, 0xA2, 0x8A,
            0xD6, 0x62, 0xA4, 0xD1, 0x8E, 0x73, 0xAF, 0xA3, 0x2D, 0x77, 0x9D, 0x59, 0x18, 0xD0, 0x8B, 0xC8,
            0x85, 0x8F, 0x4D, 0xCE, 0xF9, 0x7C, 0x2A, 0x24, 0x85, 0x5E, 0x6E, 0xEB, 0x22, 0xB3, 0xB2, 0xE5
        };

        // Mnozenie 1024-bit x 1024-bit -> 2048-bit
        BigInt2048 Mul1024(const BigInt1024& a, const BigInt1024& b) noexcept
        {
            BigInt2048 res{};
            for (size_t i = 0; i < 16; ++i)
            {
                uint64_t carry = 0;
                for (size_t j = 0; j < 16; ++j)
                {
                    uint64_t hi;
                    uint64_t lo = _umul128(a.w[i], b.w[j], &hi);

                    uint8_t c1 = _addcarry_u64(0, res.w[i + j], lo, &res.w[i + j]);
                    uint8_t c2 = _addcarry_u64(c1, hi, 0, &hi);
                    uint8_t c3 = _addcarry_u64(0, res.w[i + j], carry, &res.w[i + j]);
                    carry = hi + (c2 ? 1 : 0) + (c3 ? 1 : 0);
                }
                res.w[i + 16] = carry;
            }
            return res;
        }

        // Redukcja modularna: r % p
        BigInt1024 ModP(const BigInt2048& r, const BigInt1024& p) noexcept
        {
            BigInt1024 rem{};
            for (int bit = 2047; bit >= 0; --bit)
            {
                uint8_t bitVal = static_cast<uint8_t>((r.w[bit / 64] >> (bit % 64)) & 1);

                // Przesuniecie rem w lewo o 1 i wstawienie bitVal
                uint8_t carry = bitVal;
                for (size_t i = 0; i < 16; ++i)
                {
                    uint64_t nextCarry = rem.w[i] >> 63;
                    rem.w[i] = (rem.w[i] << 1) | carry;
                    carry = static_cast<uint8_t>(nextCarry);
                }

                // Porownanie rem z p
                bool greaterOrEqual = (carry > 0);
                if (!greaterOrEqual)
                {
                    for (int i = 15; i >= 0; --i)
                    {
                        if (rem.w[i] > p.w[i]) { greaterOrEqual = true; break; }
                        if (rem.w[i] < p.w[i]) { break; }
                        if (i == 0) { greaterOrEqual = true; } // rowne
                    }
                }

                if (greaterOrEqual)
                {
                    uint8_t borrow = 0;
                    for (size_t i = 0; i < 16; ++i)
                    {
                        borrow = _subborrow_u64(borrow, rem.w[i], p.w[i], &rem.w[i]);
                    }
                }
            }
            return rem;
        }

        // Modularne potegowanie: (base ^ exp) % p
        BigInt1024 ModPow(BigInt1024 base, const std::array<uint64_t, 3>& exp, const BigInt1024& p) noexcept
        {
            BigInt1024 result{};
            result.w[0] = 1;

            int highestBit = -1;
            for (int b = 191; b >= 0; --b)
            {
                if ((exp[b / 64] >> (b % 64)) & 1)
                {
                    highestBit = b;
                    break;
                }
            }
            if (highestBit < 0) return result; // exp == 0 -> 1

            for (int b = highestBit; b >= 0; --b)
            {
                BigInt2048 sq = Mul1024(result, result);
                result = ModP(sq, p);

                if ((exp[b / 64] >> (b % 64)) & 1)
                {
                    BigInt2048 mul = Mul1024(result, base);
                    result = ModP(mul, p);
                }
            }
            return result;
        }
    } // namespace

    bool NegotiateDH2Keys(std::span<const uint8_t> serverPubData, LuriaDH2Keys& outKeys)
    {
        if (serverPubData.size() < 256)
        {
            return false;
        }

        const BigInt1024 P = FromBytes128BE(P_BYTES);
        const BigInt1024 G = FromBytes128BE(G_BYTES);

        const BigInt1024 sPub1 = FromBytes128BE(serverPubData.data());
        const BigInt1024 sPub2 = FromBytes128BE(serverPubData.data() + 128);

        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dis;

        std::array<uint8_t, 256> agreedKey{};
        std::array<uint64_t, 3> priv1{};
        std::array<uint64_t, 3> priv2{};

        constexpr int TARGET_MOD = 11; // Blowfish CTR (c % 14 == 11)
        constexpr int MAX_ATTEMPTS = 30000;

        for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
        {
            // Losowanie kluczy prywatnych (160 bitow = 20 B)
            priv1[0] = dis(gen);
            priv1[1] = dis(gen);
            priv1[2] = dis(gen) & 0xFFFFFFFFULL; // 160 bitow
            if (priv1[0] == 0) priv1[0] = 1;

            priv2[0] = dis(gen);
            priv2[1] = dis(gen);
            priv2[2] = dis(gen) & 0xFFFFFFFFULL;
            if (priv2[0] == 0) priv2[0] = 1;

            BigInt1024 sec1 = ModPow(sPub1, priv1, P);
            BigInt1024 sec2 = ModPow(sPub2, priv2, P);

            ToBytes128BE(sec1, agreedKey.data());
            ToBytes128BE(sec2, agreedKey.data() + 128);

            int c1 = agreedKey[agreedKey[0] % 256] % 14;
            int c2 = agreedKey[agreedKey[1] % 256] % 14;

            if (c1 == TARGET_MOD && c2 == TARGET_MOD)
            {
                // Wygenerowanie klucza publicznego klienta: G^priv1 mod P, G^priv2 mod P
                BigInt1024 cPub1 = ModPow(G, priv1, P);
                BigInt1024 cPub2 = ModPow(G, priv2, P);

                ToBytes128BE(cPub1, outKeys.cgPubData.data());
                ToBytes128BE(cPub2, outKeys.cgPubData.data() + 128);

                // Ekstrakcja kluczy sesyjnych (zgodnie z LuriaCrypto.cs):
                // key1 = agreedKey[0..16], key2 = agreedKey[16..32]
                // iv1  = agreedKey[248..256], iv2 = agreedKey[240..248]
                std::memcpy(outKeys.key1.data(), agreedKey.data(), 16);
                std::memcpy(outKeys.key2.data(), agreedKey.data() + 16, 16);
                std::memcpy(outKeys.iv1.data(), agreedKey.data() + 248, 8);
                std::memcpy(outKeys.iv2.data(), agreedKey.data() + 240, 8);

                return true;
            }
        }

        return false;
    }

    bool TestModPowConsistency()
    {
        const BigInt1024 P = FromBytes128BE(P_BYTES);
        const BigInt1024 G = FromBytes128BE(G_BYTES);

        std::array<uint64_t, 3> exp = { 0x1234567890ABCDEFULL, 0xFEDCBA0987654321ULL, 0x1ULL };
        BigInt1024 res = ModPow(G, exp, P);

        uint8_t outBytes[128]{};
        ToBytes128BE(res, outBytes);

        // Oczekiwany hash / hex z Pythona:
        const char* expectedHex = 
            "96df25e8abec730eebb7292383b3a48820712ef9591f5c609437984027048067"
            "5b4dffd209cc0d2b54919a4613e8a291825db85e95633350529246bd85ad19a1"
            "69555b14accbbb2c2b21391de31cb43739fbd6945f5ab11d4ce68bf562d03345"
            "91a169dc38a348c97d1ff75e0ccec00dade12f8f06cd0c74f9f115dc01c19480";

        char hexBuf[257]{};
        for (int i = 0; i < 128; ++i)
        {
            snprintf(hexBuf + i * 2, 3, "%02x", outBytes[i]);
        }

        return (std::strcmp(hexBuf, expectedHex) == 0);
    }

} // namespace Luria

