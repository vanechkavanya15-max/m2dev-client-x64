/**
 * @file XXHash64Constexpr.h
 * @brief Blyskawiczna funkcja hashujaca XXHash64 w czasie kompilacji (constexpr).
 * 
 * Implementuje algorytm XXHash64 do szybkiego obliczania skrotow dla ciagow
 * znakow (std::string_view) i buforow (std::span) w czasie kompilacji (C++23).
 */

#pragma once

#include <cstdint>
#include <string_view>
#include <span>
#include <expected>
#include <format>
#include "StrongTypes.h"
#include "Result.h"

namespace EterBase {

/**
 * @class XXHash64
 * @brief Implementacja algorytmu XXHash64 w czasie kompilacji.
 * 
 * Oblicza szybki 64-bitowy hash dla danych. Zgodne ze standardem XXHash64.
 */
class XXHash64 {
private:
    static constexpr uint64_t PRIME64_1 = 11400714785074694791ULL;
    static constexpr uint64_t PRIME64_2 = 14029467366897019727ULL;
    static constexpr uint64_t PRIME64_3 = 1609587929392839161ULL;
    static constexpr uint64_t PRIME64_4 = 9650029242287828579ULL;
    static constexpr uint64_t PRIME64_5 = 2870177450012600261ULL;

    /**
     * @brief Wykonuje rotacje bitowa w lewo.
     * @param x Wartosc do zrotowania.
     * @param r Liczba bitow.
     * @return Zrotowana wartosc.
     */
    [[nodiscard]] static constexpr uint64_t rotl64(uint64_t x, int r) noexcept {
        return (x << r) | (x >> (64 - r));
    }

    /**
     * @brief Wykonuje jedna runde XXHash.
     * @param acc Akumulator.
     * @param val Wartosc.
     * @return Nowa wartosc akumulatora.
     */
    [[nodiscard]] static constexpr uint64_t roundXX(uint64_t acc, uint64_t val) noexcept {
        acc += val * PRIME64_2;
        acc = rotl64(acc, 31);
        acc *= PRIME64_1;
        return acc;
    }

    /**
     * @brief Scala wyniki rund.
     * @param acc Akumulator.
     * @param val Wartosc rundy.
     * @return Nowa wartosc akumulatora.
     */
    [[nodiscard]] static constexpr uint64_t mergeRound(uint64_t acc, uint64_t val) noexcept {
        val = roundXX(0, val);
        acc ^= val;
        acc = acc * PRIME64_1 + PRIME64_4;
        return acc;
    }

    /**
     * @brief Czyta pojedynczy bajt w sposob bezpieczny dla constexpr.
     * @param data Wskaznik na bufor.
     * @param idx Indeks bajtu.
     * @return Odczytany bajt.
     */
    template <typename T>
    [[nodiscard]] static constexpr uint8_t byteAt(const T* data, size_t idx) noexcept {
        return static_cast<uint8_t>(data[idx]);
    }

    /**
     * @brief Czyta 64-bitowa wartosc Little Endian z bufora.
     * @param p Wskaznik na bufor.
     * @param offset Przesuniecie w buforze.
     * @return Odczytana wartosc.
     */
    template <typename T>
    [[nodiscard]] static constexpr uint64_t read64le(const T* p, size_t offset) noexcept {
        return (static_cast<uint64_t>(byteAt(p, offset))) | 
               (static_cast<uint64_t>(byteAt(p, offset + 1)) << 8) | 
               (static_cast<uint64_t>(byteAt(p, offset + 2)) << 16) | 
               (static_cast<uint64_t>(byteAt(p, offset + 3)) << 24) |
               (static_cast<uint64_t>(byteAt(p, offset + 4)) << 32) | 
               (static_cast<uint64_t>(byteAt(p, offset + 5)) << 40) | 
               (static_cast<uint64_t>(byteAt(p, offset + 6)) << 48) | 
               (static_cast<uint64_t>(byteAt(p, offset + 7)) << 56);
    }

    /**
     * @brief Czyta 32-bitowa wartosc Little Endian z bufora.
     * @param p Wskaznik na bufor.
     * @param offset Przesuniecie w buforze.
     * @return Odczytana wartosc.
     */
    template <typename T>
    [[nodiscard]] static constexpr uint32_t read32le(const T* p, size_t offset) noexcept {
        return (static_cast<uint32_t>(byteAt(p, offset))) | 
               (static_cast<uint32_t>(byteAt(p, offset + 1)) << 8) | 
               (static_cast<uint32_t>(byteAt(p, offset + 2)) << 16) | 
               (static_cast<uint32_t>(byteAt(p, offset + 3)) << 24);
    }

    /**
     * @brief Wewnetrzna implementacja glownej petli algorytmu XXHash64.
     * @param data Wskaznik na poczatek danych.
     * @param len Dlugas danych.
     * @param seed Ziarno poczatkowe.
     * @return Wynikowy 64-bitowy hash.
     */
    template <typename T>
    [[nodiscard]] static constexpr uint64_t HashImpl(const T* data, size_t len, uint64_t seed) noexcept {
        if (!data && len > 0) {
            return 0; // Bedzie obsluzone wyzej
        }

        size_t offset = 0;
        uint64_t h64;

        if (len >= 32) {
            size_t limit = len - 32;
            uint64_t v1 = seed + PRIME64_1 + PRIME64_2;
            uint64_t v2 = seed + PRIME64_2;
            uint64_t v3 = seed + 0;
            uint64_t v4 = seed - PRIME64_1;

            while (offset <= limit) {
                v1 = roundXX(v1, read64le(data, offset)); offset += 8;
                v2 = roundXX(v2, read64le(data, offset)); offset += 8;
                v3 = roundXX(v3, read64le(data, offset)); offset += 8;
                v4 = roundXX(v4, read64le(data, offset)); offset += 8;
            }

            h64 = rotl64(v1, 1) + rotl64(v2, 7) + rotl64(v3, 12) + rotl64(v4, 18);
            h64 = mergeRound(h64, v1);
            h64 = mergeRound(h64, v2);
            h64 = mergeRound(h64, v3);
            h64 = mergeRound(h64, v4);
        } else {
            h64 = seed + PRIME64_5;
        }

        h64 += static_cast<uint64_t>(len);

        while (offset + 8 <= len) {
            uint64_t k1 = roundXX(0, read64le(data, offset));
            h64 ^= k1;
            h64 = rotl64(h64, 27) * PRIME64_1 + PRIME64_4;
            offset += 8;
        }

        if (offset + 4 <= len) {
            h64 ^= static_cast<uint64_t>(read32le(data, offset)) * PRIME64_1;
            h64 = rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
            offset += 4;
        }

        while (offset < len) {
            h64 ^= static_cast<uint64_t>(byteAt(data, offset)) * PRIME64_5;
            h64 = rotl64(h64, 11) * PRIME64_1;
            offset++;
        }

        h64 ^= h64 >> 33;
        h64 *= PRIME64_2;
        h64 ^= h64 >> 29;
        h64 *= PRIME64_3;
        h64 ^= h64 >> 32;

        return h64;
    }

public:
    /**
     * @brief Oblicza 64-bitowy hash XXHash64 dla ciagu znakow w czasie kompilacji.
     * 
     * @param text Ciag znakow do shashowania.
     * @param seed Ziarno hasha (domyslnie 0).
     * @return std::expected<uint64_t, PacketError> Obliczony hash lub blad (np. pusty i nieprawidlowy bufor).
     */
    [[nodiscard]] static constexpr std::expected<uint64_t, PacketError> Hash(std::string_view text, uint64_t seed = 0) noexcept {
        if (!text.empty() && text.data() == nullptr) {
             return std::unexpected(PacketError::BufferUnderflow);
        }
        return HashImpl(text.data(), text.size(), seed);
    }

    /**
     * @brief Oblicza 64-bitowy hash XXHash64 dla bufora w czasie kompilacji.
     * 
     * @param buffer Bufor danych do shashowania.
     * @param seed Ziarno hasha (domyslnie 0).
     * @return std::expected<uint64_t, PacketError> Obliczony hash lub blad (np. pusty i nieprawidlowy bufor).
     */
    [[nodiscard]] static constexpr std::expected<uint64_t, PacketError> Hash(std::span<const uint8_t> buffer, uint64_t seed = 0) noexcept {
         if (!buffer.empty() && buffer.data() == nullptr) {
             return std::unexpected(PacketError::BufferUnderflow);
         }
         return HashImpl(buffer.data(), buffer.size(), seed);
    }
};

} // namespace EterBase
