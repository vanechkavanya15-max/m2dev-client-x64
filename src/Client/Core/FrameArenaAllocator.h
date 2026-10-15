#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>
#include <new>
#include "EterBase/Result.h"

namespace Client::Core {

    /**
     * @class FrameArenaAllocator
     * @brief Alokator pamieci bez fragmentacji sterty z czyszczeniem na koniec klatki gry.
     * 
     * Implementacja C++23. Nie zrzuca wyjatkow przy braku pamieci (korzysta z Result<T> / std::expected).
     */
    class FrameArenaAllocator {
    public:
        static constexpr size_t DEFAULT_CAPACITY = 16 * 1024 * 1024; // 16 MB

        explicit FrameArenaAllocator(size_t capacity = DEFAULT_CAPACITY) noexcept 
            : m_buffer(new(std::nothrow) std::byte[capacity]), 
              m_capacity(m_buffer ? capacity : 0), 
              m_offset(0) {}

        ~FrameArenaAllocator() = default;

        // Brak mozliwosci kopiowania
        FrameArenaAllocator(const FrameArenaAllocator&) = delete;
        FrameArenaAllocator& operator=(const FrameArenaAllocator&) = delete;

        // Przenoszenie
        FrameArenaAllocator(FrameArenaAllocator&& other) noexcept 
            : m_buffer(std::move(other.m_buffer)),
              m_capacity(std::exchange(other.m_capacity, 0)),
              m_offset(std::exchange(other.m_offset, 0)) {}

        FrameArenaAllocator& operator=(FrameArenaAllocator&& other) noexcept {
            if (this != &other) {
                m_buffer = std::move(other.m_buffer);
                m_capacity = std::exchange(other.m_capacity, 0);
                m_offset = std::exchange(other.m_offset, 0);
            }
            return *this;
        }

        [[nodiscard]] EterBase::Result<std::span<std::byte>> Allocate(size_t size, size_t alignment = alignof(std::max_align_t)) noexcept {
            if (!m_buffer) {
                return EterBase::MakeError("Allocator not initialized (OOM during creation)");
            }

            if (size == 0) {
                return EterBase::MakeError("Zero size allocation requested");
            }

            void* current_ptr = m_buffer.get() + m_offset;
            size_t space = m_capacity - m_offset;

            void* aligned_ptr = std::align(alignment, size, current_ptr, space);
            if (!aligned_ptr) {
                return EterBase::MakeError("Out of memory in FrameArenaAllocator");
            }

            std::byte* start = static_cast<std::byte*>(aligned_ptr);
            m_offset = (start - m_buffer.get()) + size;

            return std::span<std::byte>(start, size);
        }

        template <typename T, typename... Args>
        [[nodiscard]] EterBase::Result<T*> AllocateObject(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
            auto result = Allocate(sizeof(T), alignof(T));
            if (!result.has_value()) {
                return EterBase::MakeError(result.error());
            }

            return new (result.value().data()) T(std::forward<Args>(args)...);
        }
        
        template<typename T>
        [[nodiscard]] EterBase::Result<std::span<T>> AllocateArray(size_t count) noexcept(std::is_nothrow_default_constructible_v<T>) {
            if (count == 0) {
                 return EterBase::MakeError("Zero count array allocation requested");
            }
            
            auto result = Allocate(sizeof(T) * count, alignof(T));
            if (!result.has_value()) {
                return EterBase::MakeError(result.error());
            }
            
            T* ptr = reinterpret_cast<T*>(result.value().data());
            
            // Inicjalizacja pamieci uzywajac std::uninitialized_default_construct_n dla bezpieczenstwa wyjatkow
            std::uninitialized_default_construct_n(ptr, count);
            
            return std::span<T>(ptr, count);
        }

        void Reset() noexcept {
            m_offset = 0;
        }

        [[nodiscard]] size_t GetAllocatedBytes() const noexcept {
            return m_offset;
        }

        [[nodiscard]] size_t GetCapacity() const noexcept {
            return m_capacity;
        }

    private:
        std::unique_ptr<std::byte[]> m_buffer;
        size_t m_capacity;
        size_t m_offset;
    };

} // namespace Client::Core
