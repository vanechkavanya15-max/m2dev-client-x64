#pragma once

#include <cstddef>

#include <span>
#include <array>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <memory>
#include "../../EterBase/Result.h"

namespace Client::Network {

/**
 * @class ByteStreamWriter
 * @brief Zero-Allocation serializator pakietow klienta bazujacy na buforze stosowym.
 * 
 * Umozliwia bezpieczny, bezkopiowy zapis danych do wewnetrznego bufora o stalym rozmiarze.
 * Zwraca EterBase::PacketResult w przypadku bledu (np. przepelnienia bufora),
 * zachowujac pelne bezpieczenstwo pamieci bez uzycia sterty.
 *
 * @tparam Capacity Rozmiar wewnetrznego bufora stosowego w bajtach.
 */
template <size_t Capacity>
class ByteStreamWriter {
public:
    constexpr ByteStreamWriter() noexcept = default;
    ~ByteStreamWriter() = default;

    ByteStreamWriter(const ByteStreamWriter&) = delete;
    ByteStreamWriter& operator=(const ByteStreamWriter&) = delete;

    ByteStreamWriter(ByteStreamWriter&&) = delete;
    ByteStreamWriter& operator=(ByteStreamWriter&&) = delete;

    /**
     * @brief Zapisuje trywialnie kopiowalny typ danych do bufora.
     * @tparam T Typ danych do zapisu.
     * @param value Wartosc do zapisania.
     * @return PacketResult sygnalizujacy sukces (void) lub blad w przypadku braku miejsca.
     */
    template <typename T>
    requires std::is_trivially_copyable_v<T>
    EterBase::PacketResult<void> Write(const T& value) noexcept {
        constexpr size_t size = sizeof(T);
        if (m_offset + size > Capacity) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        std::memcpy(m_buffer.data() + m_offset, std::addressof(value), size);
        m_offset += size;
        return {};
    }

    /**
     * @brief Zapisuje ciag bajtow (span) do bufora.
     * @param data Widok na dane do zapisania.
     * @return PacketResult sygnalizujacy sukces lub blad w przypadku braku miejsca.
     */
    EterBase::PacketResult<void> WriteSpan(std::span<const uint8_t> data) noexcept {
        if (m_offset + data.size() > Capacity) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        if (!data.empty()) {
            std::memcpy(m_buffer.data() + m_offset, data.data(), data.size());
        }
        
        m_offset += data.size();
        return {};
    }

    /**
     * @brief Zwraca widok na zapisane dane w buforze.
     * @return std::span zawierajacy tylko faktycznie zapisane bajty.
     */
    [[nodiscard]] constexpr std::span<const uint8_t> GetSpan() const noexcept {
        return std::span<const uint8_t>(m_buffer.data(), m_offset);
    }

    /**
     * @brief Zwraca aktualny rozmiar zapisanych danych.
     * @return Liczba zapisanych bajtow.
     */
    [[nodiscard]] constexpr size_t GetSize() const noexcept {
        return m_offset;
    }

    /**
     * @brief Zwraca maksymalna pojemnosc bufora.
     * @return Pojemnosc bufora.
     */
    [[nodiscard]] constexpr size_t GetCapacity() const noexcept {
        return Capacity;
    }

    /**
     * @brief Zwraca ilosc pozostalego miejsca w buforze.
     * @return Liczba bajtow, ktore mozna jeszcze zapisac.
     */
    [[nodiscard]] constexpr size_t GetRemainingSpace() const noexcept {
        return Capacity - m_offset;
    }

    /**
     * @brief Cysci bufor i resetuje pozycje zapisu do zera.
     */
    constexpr void Clear() noexcept {
        m_offset = 0;
    }

private:
    std::array<uint8_t, Capacity> m_buffer{};
    size_t m_offset{0};
};

} // namespace Client::Network
