#pragma once

#include <span>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <algorithm>

namespace Client::Network {

/**
 * @class ZeroCopyPacketBuffer
 * @brief Zero-Allocation, Zero-Copy Ring Packet Buffer dla protokolu sieciowego (Standard C++23).
 * 
 * Zapewnia bezposredni dostep wskaznikowy do struktur pakietow (const TPacket*)
 * oraz widokow std::span<const uint8_t> bez zadnego kopiowania pamieci RAM (Zero-Copy)
 * i bez zaleznosci od alokacji sterty w petli odbioru sieciowego.
 */
class ZeroCopyPacketBuffer {
public:
    static constexpr size_t DEFAULT_CAPACITY = 2 * 1024 * 1024; // 2 MB
    static constexpr size_t SCRATCH_CAPACITY = 64 * 1024;        // 64 KB dla rzadkich wrap-around

    explicit ZeroCopyPacketBuffer(size_t capacity = DEFAULT_CAPACITY);
    ~ZeroCopyPacketBuffer() = default;

    // Non-copyable
    ZeroCopyPacketBuffer(const ZeroCopyPacketBuffer&) = delete;
    ZeroCopyPacketBuffer& operator=(const ZeroCopyPacketBuffer&) = delete;

    // Movable
    ZeroCopyPacketBuffer(ZeroCopyPacketBuffer&& other) noexcept;
    ZeroCopyPacketBuffer& operator=(ZeroCopyPacketBuffer&& other) noexcept;

    // --- Stan i Pojemnosc ---
    [[nodiscard]] size_t ReadableBytes() const noexcept;
    [[nodiscard]] size_t WritableBytes() const noexcept;
    [[nodiscard]] size_t Capacity() const noexcept { return m_capacity; }
    [[nodiscard]] bool HasBytes(size_t count) const noexcept { return ReadableBytes() >= count; }
    [[nodiscard]] bool IsEmpty() const noexcept { return ReadableBytes() == 0; }
    void Clear() noexcept;

    // --- Zapis (Socket Recv -> Buffer) ---
    /**
     * @brief Zwraca ciagly bufor zapisu bezposrednio dla funkcji socketu recv().
     * @param maxRequested Maksymalna liczba bajtow do odczytania.
     * @return Widok pamieci, do ktorej mozna wpisac odebrane bajty z gniazda.
     */
    [[nodiscard]] std::span<uint8_t> GetWritableSpan(size_t maxRequested = 65536) noexcept;

    /**
     * @brief Zatwierdza liczbe wpisanych bajtow po udanym recv().
     */
    void CommitWrite(size_t bytesWritten) noexcept;

    /**
     * @brief Kopiuje dane do bufora pierscieniowego (dla syntetycznych pakietow lub symulatora).
     */
    bool Write(std::span<const uint8_t> data) noexcept;

    // --- Odczyt Zero-Copy (Buffer -> Game Logic) ---
    /**
     * @brief Zwraca ciagly widok std::span na dane pakietu bez kopiowania pamieci.
     */
    [[nodiscard]] std::span<const uint8_t> PeekContiguous(size_t size) const noexcept;

    /**
     * @brief Szablon rzutujacy bezposrednio pamiec bufora na strukture pakietu bez kopiowania.
     */
    template <typename TPacket>
    [[nodiscard]] const TPacket* PeekPacket() const noexcept {
        constexpr size_t packetSize = sizeof(TPacket);
        if (!HasBytes(packetSize)) {
            return nullptr;
        }

        auto span = PeekContiguous(packetSize);
        if (span.size() < packetSize) {
            return nullptr;
        }

        return reinterpret_cast<const TPacket*>(span.data());
    }

    /**
     * @brief Zwraca wskaznik do struktury pakietu i natychmiast przesuwa wskaznik odczytu.
     */
    template <typename TPacket>
    [[nodiscard]] const TPacket* RecvPacket() noexcept {
        const TPacket* packet = PeekPacket<TPacket>();
        if (!packet) {
            return nullptr;
        }

        CommitRead(sizeof(TPacket));
        return packet;
    }

    /**
     * @brief Przesuwa wskaznik odczytu o zadana liczbe bajtow (zatwierdzenie przetworzenia).
     */
    bool CommitRead(size_t size) noexcept;

    // --- Kompatybilnosc wsteczna (memcpy fallback) ---
    bool Peek(std::span<uint8_t> outDest) const noexcept;
    bool Read(std::span<uint8_t> outDest) noexcept;

    // --- Telemetria i Statystyki ---
    [[nodiscard]] uint64_t GetZeroCopyHits() const noexcept { return m_zeroCopyHits; }
    [[nodiscard]] uint64_t GetWrapAroundFallbacks() const noexcept { return m_wrapAroundFallbacks; }
    [[nodiscard]] double GetZeroCopyRatio() const noexcept;

private:
    std::vector<uint8_t> m_buffer;
    size_t m_capacity;
    size_t m_readPos;
    size_t m_writePos;
    bool m_isFull;

    // Scratchpad uzywany wylacznie w rzadkim przypadku (wrap-around przecinajacy koniec bufora)
    mutable std::vector<uint8_t> m_scratchBuffer;

    mutable uint64_t m_zeroCopyHits{0};
    mutable uint64_t m_wrapAroundFallbacks{0};
};

} // namespace Client::Network
