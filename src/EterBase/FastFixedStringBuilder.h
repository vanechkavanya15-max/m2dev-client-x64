#pragma once

#include <cstdint>
#include <array>
#include <string_view>
#include <format>
#include <algorithm>
#include <utility>
#include "Result.h"

/**
 * @file FastFixedStringBuilder.h
 * @brief Bezpieczny bufor tekstowy na stosie z pelna integracja std::format.
 * 
 * Klasa szablonowa sluzaca do budowania stringow na stosie (bez alokacji na stercie).
 * Wykorzystuje std::format oraz std::expected (przez EterBase::VoidResult) do obslugi bledow,
 * zastepujac narazone na bledy snprintf.
 */

namespace EterBase {

/**
 * @brief Szablon bufora tekstowego alokowanego na stosie.
 * 
 * @tparam Capacity Maksymalny rozmiar bufora (wraz ze znakiem konca ciagu).
 */
template <std::size_t Capacity>
class FastFixedStringBuilder {
public:
    static_assert(Capacity > 0, "Capacity must be greater than 0");

    /**
     * @brief Konstruktor domyslny, inicjalizuje pusty bufor.
     */
    constexpr FastFixedStringBuilder() noexcept {
        Clear();
    }

    /**
     * @brief Czysci zawartosc bufora.
     */
    constexpr void Clear() noexcept {
        size = 0;
        buffer[0] = '\0';
    }

    /**
     * @brief Dodaje tekst na koniec obecnego bufora.
     * 
     * @param str Tekst do dodania.
     * @return std::expected<void, std::string_view> Wynik operacji. Zwraca blad jesli brakuje miejsca.
     */
    constexpr std::expected<void, std::string_view> Append(std::string_view str) noexcept {
        if (size + str.length() >= Capacity) {
            return MakeError(std::string_view("Buffer overflow: Cannot append string, exceeded capacity."));
        }

        std::copy(str.begin(), str.end(), buffer.begin() + size);
        size += str.length();
        buffer[size] = '\0';
        
        return {};
    }

    /**
     * @brief Formatuje i dopisuje tekst na koniec bufora uzywajac std::format.
     * 
     * @tparam Args Typy argumentow formatowania.
     * @param fmt Ciag formatujacy (sprawdzany w czasie kompilacji).
     * @param args Argumenty formatowania.
     * @return std::expected<void, std::string_view> Wynik operacji. Zwraca blad w przypadku przepelnienia lub bledu formatowania.
     */
    template <typename... Args>
    std::expected<void, std::string_view> Format(std::format_string<Args...> fmt, Args&&... args) {
        try {
            const std::size_t remainingCapacity = Capacity - size - 1; // Zostaw miejsce na '\0'
            auto result = std::format_to_n(buffer.data() + size, remainingCapacity, fmt, std::forward<Args>(args)...);
            
            if (static_cast<std::size_t>(result.size) > remainingCapacity) {
                buffer[size] = '\0'; // Restore state
                return MakeError(std::string_view("Buffer overflow: Formatted string exceeds remaining capacity."));
            }
            
            size += static_cast<std::size_t>(result.size);
            buffer[size] = '\0';
            
            return {};
        } catch (const std::format_error&) {
            buffer[size] = '\0'; // Restore state
            return MakeError(std::string_view("Format error: Invalid format string or arguments."));
        }
    }

    /**
     * @brief Zwraca widok na obecna zawartosc bufora.
     * 
     * @return std::string_view Widok tekstu.
     */
    [[nodiscard]] constexpr std::string_view View() const noexcept {
        return std::string_view(buffer.data(), size);
    }

    /**
     * @brief Zwraca C-string z zawartoscia bufora.
     * 
     * @return const char* Wskaznik na zakonczony zerem ciag znakow.
     */
    [[nodiscard]] constexpr const char* CStr() const noexcept {
        return buffer.data();
    }

    /**
     * @brief Zwraca aktualny rozmiar zapisanego tekstu.
     * 
     * @return std::size_t Liczba znakow w buforze.
     */
    [[nodiscard]] constexpr std::size_t Size() const noexcept {
        return size;
    }

    /**
     * @brief Zwraca calkowita pojemnosc bufora.
     * 
     * @return std::size_t Maksymalna pojemnosc minus jeden.
     */
    [[nodiscard]] constexpr std::size_t MaxSize() const noexcept {
        return Capacity - 1;
    }

private:
    std::array<char, Capacity> buffer{};
    std::size_t size{0};
};

} // namespace EterBase
