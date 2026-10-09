#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <expected>
#include <system_error>
#include <string>
#include <cstring>
#include <type_traits>
#include <algorithm>

namespace Client::Network
{
    /**
     * @brief Czytnik strumienia binarnego z rygorystycznym sprawdzaniem dopuszczalnego rozmiaru.
     * Wykorzystuje semantyke Zero-Copy oparta na std::span.
     */
    class ByteStreamReader
    {
    public:
        /**
         * @brief Konstruktor inicjalizujacy czytnik za pomoca bufora wejsciowego.
         * @param buffer Widok na ciagly bufor pamieci w trybie tylko do odczytu.
         */
        constexpr explicit ByteStreamReader(std::span<const uint8_t> buffer) noexcept
            : m_buffer(buffer), m_offset(0)
        {
        }

        /**
         * @brief Zwraca liczbe bajtow pozostalych do odczytu w strumieniu.
         * @return Liczba pozostalych bajtow.
         */
        [[nodiscard]] constexpr size_t GetRemainingSize() const noexcept
        {
            return m_buffer.size() - m_offset;
        }

        /**
         * @brief Sprawdza, czy w strumieniu znajduje sie wymagana liczba bajtow.
         * @param size Wymagana liczba bajtow.
         * @return true, jezeli mozna bezpiecznie odczytac dana liczbe bajtow, w przeciwnym razie false.
         */
        [[nodiscard]] constexpr bool HasEnoughBytes(size_t size) const noexcept
        {
            return GetRemainingSize() >= size;
        }

        /**
         * @brief Odczytuje surowe typy danych ze strumienia.
         * Wymaga, aby typ T byl typem prostym (TriviallyCopyable).
         * @tparam T Typ danych do odczytu.
         * @return Pomyslny odczyt zwraca wartosc, w razie bledu zwracany jest std::errc.
         */
        template <typename T>
        requires std::is_trivially_copyable_v<T>
        [[nodiscard]] std::expected<T, std::errc> Read() noexcept
        {
            if (!HasEnoughBytes(sizeof(T)))
            {
                return std::unexpected(std::errc::no_buffer_space);
            }

            T result{};
            // Uzycie memcpy dla bezpieczenstwa typow i obslugi wyrownania (alignment)
            std::memcpy(&result, m_buffer.data() + m_offset, sizeof(T));
            m_offset += sizeof(T);
            return result;
        }

        /**
         * @brief Odczytuje surowe bajty do przekazanego bufora docelowego.
         * @param out_buffer Widok na bufor docelowy, ktory ma zostac zapelniony.
         * @return true jezeli operacja sie powiodla, std::errc jezeli jest zbyt malo danych.
         */
        [[nodiscard]] std::expected<void, std::errc> ReadBytes(std::span<uint8_t> out_buffer) noexcept
        {
            if (out_buffer.empty())
            {
                return {};
            }

            if (!HasEnoughBytes(out_buffer.size()))
            {
                return std::unexpected(std::errc::no_buffer_space);
            }

            std::memcpy(out_buffer.data(), m_buffer.data() + m_offset, out_buffer.size());
            m_offset += out_buffer.size();
            return {};
        }

        /**
         * @brief Odczytuje ciag znakow (string) o maksymalnej zadanej dlugosci.
         * Zapobiega czytaniu znakow poza limitem dlugosci lub brakiem null-terminatora (jezeli wymagany).
         * 
         * @param max_length Maksymalna dlugosc ciagu w bajtach do odczytu ze strumienia.
         * @return std::string z odczytanym tekstem, ucietym na pierwszym znaku \0 jezeli istnieje.
         */
        [[nodiscard]] std::expected<std::string, std::errc> ReadString(size_t max_length)
        {
            if (!HasEnoughBytes(max_length))
            {
                return std::unexpected(std::errc::no_buffer_space);
            }

            auto start_ptr = reinterpret_cast<const char*>(m_buffer.data() + m_offset);
            
            // Szuka null-terminatora lub ogranicza dlugosc do max_length
            size_t actual_length = 0;
            while (actual_length < max_length && start_ptr[actual_length] != '\0')
            {
                ++actual_length;
            }

            std::string result(start_ptr, actual_length);
            m_offset += max_length; // Konsumuje cala przestrzen w strumieniu wg specyfikacji protokolu np dla 64-bajtowych stringow z gory
            return result;
        }

    private:
        std::span<const uint8_t> m_buffer;
        size_t m_offset;
    };

} // namespace Client::Network
