#pragma once

#include "../../EterBase/Result.h"
#include <vector>
#include <span>
#include <cstdint>

namespace Client::Network {

/**
 * @class PacketRingBuffer
 * @brief Bezalokacyjny bufor cykliczny (Ring Buffer) o gwarantowanym stalym rozmiarze
 *        dla pakietow sieciowych. Uzywa C++23 std::expected (EterBase::PacketResult).
 */
class PacketRingBuffer {
public:
    explicit PacketRingBuffer(size_t capacity);

    // Zapobiegamy kopiowaniu
    PacketRingBuffer(const PacketRingBuffer&) = delete;
    PacketRingBuffer& operator=(const PacketRingBuffer&) = delete;

    // Przenoszenie dozwolone
    PacketRingBuffer(PacketRingBuffer&&) noexcept = default;
    PacketRingBuffer& operator=(PacketRingBuffer&&) noexcept = default;

    /**
     * @brief Zapisuje dane do bufora cyklicznego.
     * @param data Dane do zapisania.
     * @return EterBase::VoidResult przy sukcesie, EterBase::PacketError::BufferUnderflow przy braku miejsca.
     */
    [[nodiscard]] EterBase::VoidResult<EterBase::PacketError> Write(std::span<const uint8_t> data);

    /**
     * @brief Odczytuje dane z bufora i przesuwa wskaznik odczytu.
     * @param outBuffer Bufor wyjsciowy.
     * @return EterBase::VoidResult przy sukcesie, blad w przeciwnym razie.
     */
    [[nodiscard]] EterBase::VoidResult<EterBase::PacketError> Read(std::span<uint8_t> outBuffer);

    /**
     * @brief Podglada dane z bufora bez przesuwania wskaznika odczytu.
     * @param outBuffer Bufor wyjsciowy.
     * @return EterBase::VoidResult przy sukcesie, blad w przeciwnym razie.
     */
    [[nodiscard]] EterBase::VoidResult<EterBase::PacketError> Peek(std::span<uint8_t> outBuffer) const;

    /**
     * @brief Pomija (przesuwa wskaznik odczytu) podana liczbe bajtow.
     * @param size Liczba bajtow do pominiecia.
     * @return EterBase::VoidResult przy sukcesie, blad w przeciwnym razie.
     */
    [[nodiscard]] EterBase::VoidResult<EterBase::PacketError> Skip(size_t size);

    /**
     * @brief Zwraca liczbe bajtow dostepnych do odczytu (dane zajmujace bufor).
     */
    [[nodiscard]] size_t GetAvailableSize() const noexcept;

    /**
     * @brief Zwraca liczbe bajtow dostepnych do zapisu (wolne miejsce w buforze).
     */
    [[nodiscard]] size_t GetFreeSize() const noexcept;
    
    /**
     * @brief Weryfikuje czy dostepna jest wystarczajaca ilosc danych dla pakietu o dynamicznym rozmiarze.
     * @param expectedSize Oczekiwany rozmiar calego pakietu.
     * @return EterBase::VoidResult przy sukcesie, EterBase::PacketError::BufferUnderflow jesli brakuje danych.
     */
    [[nodiscard]] EterBase::VoidResult<EterBase::PacketError> PeekDynamicSize(size_t expectedSize) const;

    /**
     * @brief Zwraca calkowita pojemnosc bufora.
     */
    [[nodiscard]] size_t GetCapacity() const noexcept;

private:
    std::vector<uint8_t> m_buffer;
    size_t m_readPos;
    size_t m_writePos;
    size_t m_capacity;
    bool m_isFull;
};

} // namespace Client::Network
