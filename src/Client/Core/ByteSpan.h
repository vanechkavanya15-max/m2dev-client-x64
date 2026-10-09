#pragma once

#include <span>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <type_traits>
#include <bit>
#include <array>
#include "Result.h"

namespace Client::Core {

// Definiujemy wlasne typy spana dla bajtow, by jasno sygnalizowac ich intencje w kodzie
using ByteSpan = std::span<std::byte>;
using ReadOnlyByteSpan = std::span<const std::byte>;

/**
 * @brief Bezpieczny ekstraktor z bufora. Uzywany w Zero-Copy Data Pipeline.
 */
class SpanReader {
public:
    constexpr explicit SpanReader(ReadOnlyByteSpan span) noexcept : m_buffer(span) {}

    /**
     * @brief Odczytuje surowy wycinek bajtow.
     * @param size Rozmiar w bajtach.
     * @return Zwraca spana na zadany rozmiar.
     */
    [[nodiscard]] constexpr Result<ReadOnlyByteSpan, PacketError> ReadSpan(size_t size) noexcept {
        if (size > m_buffer.size()) {
            return std::unexpected(PacketError::BufferUnderflow);
        }
        auto result = m_buffer.first(size);
        m_buffer = m_buffer.subspan(size);
        return result;
    }

    /**
     * @brief Odczytuje obiekt w sposob bezpieczny. W pelni constexpr dla C++20 przy uzyciu std::bit_cast.
     * @tparam T Typ, jaki ma zostac odczytany z bufora. Musi to byc typ standardowego ukladu i trywialny.
     * @return Odczytana wartosc.
     */
    template <typename T>
    [[nodiscard]] constexpr Result<T, PacketError> Read() noexcept {
        static_assert(std::is_standard_layout_v<T> && std::is_trivial_v<T>, "T must be standard layout and trivial");
        if (sizeof(T) > m_buffer.size()) {
            return std::unexpected(PacketError::BufferUnderflow);
        }

        std::array<std::byte, sizeof(T)> temp_buffer{};
        for(size_t i = 0; i < sizeof(T); ++i) {
            temp_buffer[i] = m_buffer[i];
        }

        m_buffer = m_buffer.subspan(sizeof(T));
        return std::bit_cast<T>(temp_buffer);
    }

    [[nodiscard]] constexpr size_t Remaining() const noexcept {
        return m_buffer.size();
    }

private:
    ReadOnlyByteSpan m_buffer;
};

/**
 * @brief Klasa obudowujaca pisanie do spanu bajtow.
 */
class SpanWriter {
public:
    constexpr explicit SpanWriter(ByteSpan span) noexcept : m_buffer(span) {}

    /**
     * @brief Zapisuje surowy wycinek bajtow.
     */
    constexpr Result<void, PacketError> WriteSpan(ReadOnlyByteSpan data) noexcept {
        if (data.size() > m_buffer.size()) {
            return std::unexpected(PacketError::BufferUnderflow);
        }
        for (size_t i = 0; i < data.size(); ++i) {
            m_buffer[i] = data[i];
        }
        m_buffer = m_buffer.subspan(data.size());
        return {};
    }

    /**
     * @brief Zapisuje obiekt bezpiecznie. W pelni constexpr.
     * @tparam T Typ obiektu. Musi to byc typ o standardowym ukladzie i trywialny.
     * @param value Wartosc do zapisania.
     */
    template <typename T>
    constexpr Result<void, PacketError> Write(const T& value) noexcept {
        static_assert(std::is_standard_layout_v<T> && std::is_trivial_v<T>, "T must be standard layout and trivial");
        if (sizeof(T) > m_buffer.size()) {
            return std::unexpected(PacketError::BufferUnderflow);
        }

        auto temp_buffer = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
        for(size_t i = 0; i < sizeof(T); ++i) {
            m_buffer[i] = temp_buffer[i];
        }

        m_buffer = m_buffer.subspan(sizeof(T));
        return {};
    }

    [[nodiscard]] constexpr size_t Remaining() const noexcept {
        return m_buffer.size();
    }

private:
    ByteSpan m_buffer;
};

} // namespace Client::Core
