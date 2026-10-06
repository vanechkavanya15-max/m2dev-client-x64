#pragma once

#include <atomic>
#include <vector>
#include <span>
#include <cstdint>
#include <algorithm>
#include <expected>
#include "Result.h"
#include "LogModern.h"

/**
 * @file FastLockFreeRingBuffer.h
 * @brief Bez-zamkowa (lock-free) kolejka kołowa (Single-Producer, Single-Consumer) zoptymalizowana dla C++23.
 * Zapewnia zero-GC (brak alokacji podczas działania) dzięki wstępnie zaalokowanej pamięci.
 */

namespace EterBase {

/**
 * @class FastLockFreeRingBuffer
 * @brief Lock-free ring buffer dla strumieni danych, np. pakietów sieciowych.
 * Aktualizuje wyłącznie stan w pamięci C++ i opiera się na atomowych barierach pamięci zamiast mutexach.
 */
class FastLockFreeRingBuffer {
public:
    /**
     * @brief Inicjuje bufor o podanej użytecznej pojemności.
     * @param capacity Użyteczny rozmiar bufora w bajtach.
     */
    explicit FastLockFreeRingBuffer(size_t capacity)
        : m_capacity(std::max<size_t>(1, capacity) + 1),
          m_buffer(m_capacity, 0),
          m_head(0),
          m_tail(0) {
    }

    /**
     * @brief Zapisuje dane do bufora (SPSC).
     * @param data Zestaw bajtów (span) do zapisania w buforze.
     * @return PacketResult<void> informujący o sukcesie lub błędzie (np. PacketError::BufferUnderflow przy przepełnieniu).
     */
    [[nodiscard]] PacketResult<void> Write(std::span<const uint8_t> data) noexcept {
        const size_t current_tail = m_tail.load(std::memory_order_relaxed);
        const size_t current_head = m_head.load(std::memory_order_acquire);
        
        size_t used_space = (current_tail >= current_head) ? 
                            (current_tail - current_head) : 
                            (m_capacity - current_head + current_tail);
                            
        size_t free_space = m_capacity - 1 - used_space;

        if (data.size() > free_space) {
            return MakeError(PacketError::BufferUnderflow);
        }

        size_t first_part = std::min(data.size(), m_capacity - current_tail);
        std::copy_n(data.data(), first_part, m_buffer.data() + current_tail);
        
        if (first_part < data.size()) {
            std::copy_n(data.data() + first_part, data.size() - first_part, m_buffer.data());
        }
        
        m_tail.store((current_tail + data.size()) % m_capacity, std::memory_order_release);
        return {};
    }

    /**
     * @brief Odczytuje i usuwa dane z bufora.
     * @param dest Bufor docelowy (span) do zapisu odczytanych danych.
     * @return PacketResult<void> informujący o sukcesie lub błędzie braku danych.
     */
    [[nodiscard]] PacketResult<void> Read(std::span<uint8_t> dest) noexcept {
        auto res = Peek(dest);
        if (!res.has_value()) {
            return res;
        }
        return Skip(dest.size());
    }

    /**
     * @brief Podgląda dane z bufora bez przesuwania wskaźnika odczytu (zero-copy w logice strumienia).
     * @param dest Bufor docelowy (span), w którym skopiowane zostaną dane.
     * @return PacketResult<void> informujący o sukcesie lub braku odpowiedniej ilości danych.
     */
    [[nodiscard]] PacketResult<void> Peek(std::span<uint8_t> dest) const noexcept {
        const size_t current_head = m_head.load(std::memory_order_relaxed);
        const size_t current_tail = m_tail.load(std::memory_order_acquire);
        
        size_t current_size = (current_tail >= current_head) ? 
                              (current_tail - current_head) : 
                              (m_capacity - current_head + current_tail);

        if (dest.size() > current_size) {
            return MakeError(PacketError::BufferUnderflow);
        }

        size_t first_part = std::min(dest.size(), m_capacity - current_head);
        std::copy_n(m_buffer.data() + current_head, first_part, dest.data());
        
        if (first_part < dest.size()) {
            std::copy_n(m_buffer.data(), dest.size() - first_part, dest.data() + first_part);
        }
        
        return {};
    }

    /**
     * @brief Pomija określoną liczbę bajtów w buforze, zwalniając to miejsce (np. po pomyślnym podglądzie/parsowaniu).
     * @param count Ilość bajtów do ominięcia (usunięcia).
     * @return PacketResult<void> informujący o poprawnym pominięciu bajtów.
     */
    [[nodiscard]] PacketResult<void> Skip(size_t count) noexcept {
        const size_t current_head = m_head.load(std::memory_order_relaxed);
        const size_t current_tail = m_tail.load(std::memory_order_acquire);
        
        size_t current_size = (current_tail >= current_head) ? 
                              (current_tail - current_head) : 
                              (m_capacity - current_head + current_tail);

        if (count > current_size) {
            return MakeError(PacketError::BufferUnderflow);
        }
        
        m_head.store((current_head + count) % m_capacity, std::memory_order_release);
        return {};
    }

    /**
     * @brief Zwraca ilość zapisanych (dostępnych do odczytu) bajtów.
     * @return Ilość danych gotowych do odczytu.
     */
    [[nodiscard]] size_t GetSize() const noexcept {
        const size_t current_head = m_head.load(std::memory_order_relaxed);
        const size_t current_tail = m_tail.load(std::memory_order_acquire);
        
        if (current_tail >= current_head) {
            return current_tail - current_head;
        }
        return m_capacity - current_head + current_tail;
    }

    /**
     * @brief Zwraca ilość wolnego miejsca w buforze do zapisu.
     * @return Ilość wolnych bajtów.
     */
    [[nodiscard]] size_t GetFreeSpace() const noexcept {
        return m_capacity - 1 - GetSize();
    }

    /**
     * @brief Czyści zawartość bufora poprzez zresetowanie wskaźników.
     */
    void Clear() noexcept {
        m_head.store(0, std::memory_order_relaxed);
        m_tail.store(0, std::memory_order_relaxed);
    }

private:
    size_t m_capacity;
    std::vector<uint8_t> m_buffer; 
    
    // Wyrównanie do rozmiaru linii pamięci podręcznej w celu uniknięcia fałszywego współdzielenia (false sharing)
    alignas(64) std::atomic<size_t> m_head;
    alignas(64) std::atomic<size_t> m_tail;
};

} // namespace EterBase
