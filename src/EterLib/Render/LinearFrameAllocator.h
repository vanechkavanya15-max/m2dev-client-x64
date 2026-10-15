#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>
#include <new>
#include <utility>

namespace EterLib::Render
{
    class LinearFrameAllocator
    {
    public:
        // Default capacity set to 16 MB
        static constexpr size_t DEFAULT_CAPACITY = 16 * 1024 * 1024;

        explicit LinearFrameAllocator(size_t capacity = DEFAULT_CAPACITY);
        ~LinearFrameAllocator();

        // Non-copyable
        LinearFrameAllocator(const LinearFrameAllocator&) = delete;
        LinearFrameAllocator& operator=(const LinearFrameAllocator&) = delete;

        // Movable
        LinearFrameAllocator(LinearFrameAllocator&& other) noexcept;
        LinearFrameAllocator& operator=(LinearFrameAllocator&& other) noexcept;

        [[nodiscard]] void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t)) noexcept;

        template <typename T, typename... Args>
        [[nodiscard]] T* AllocateObject(Args&&... args)
        {
            void* memory = Allocate(sizeof(T), alignof(T));
            if (!memory)
            {
                return nullptr;
            }

            return new (memory) T(std::forward<Args>(args)...);
        }

        void Reset() noexcept;

        [[nodiscard]] size_t GetAllocatedBytes() const noexcept;
        [[nodiscard]] size_t GetCapacity() const noexcept;

    private:
        std::unique_ptr<std::byte[]> m_buffer;
        size_t m_capacity;
        size_t m_offset;
    };
} // namespace EterLib::Render

