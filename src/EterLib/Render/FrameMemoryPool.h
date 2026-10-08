#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <vector>
#include <memory>
#include <utility>
#include <type_traits>

namespace EterLib::Render
{
    /**
     * @brief Pula pamieci o stalej wielkosci elementu zoptymalizowana pod katem linii pamieci podrecznej L2/L3 (64 bajty).
     * Wykorzystuje wewnetrzna liste wolnych slotow (intrusive free-list) bez dodatkowego narzutu na naglowki.
     */
    template <typename T, size_t BlockSize = 1024>
    class FrameMemoryPool
    {
        static_assert(BlockSize > 0, "BlockSize must be greater than zero");

        union Slot
        {
            Slot* next;
            alignas(alignof(T)) std::byte storage[sizeof(T)];
        };

        struct alignas(64) MemoryBlock
        {
            Slot slots[BlockSize];
        };

    public:
        using value_type = T;

        FrameMemoryPool() = default;

        ~FrameMemoryPool()
        {
            Reset();
        }

        // Non-copyable
        FrameMemoryPool(const FrameMemoryPool&) = delete;
        FrameMemoryPool& operator=(const FrameMemoryPool&) = delete;

        // Movable
        FrameMemoryPool(FrameMemoryPool&& other) noexcept
            : m_blocks(std::move(other.m_blocks))
            , m_freeList(other.m_freeList)
            , m_activeCount(other.m_activeCount)
            , m_capacity(other.m_capacity)
        {
            other.m_freeList = nullptr;
            other.m_activeCount = 0;
            other.m_capacity = 0;
        }

        FrameMemoryPool& operator=(FrameMemoryPool&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                m_blocks = std::move(other.m_blocks);
                m_freeList = other.m_freeList;
                m_activeCount = other.m_activeCount;
                m_capacity = other.m_capacity;

                other.m_freeList = nullptr;
                other.m_activeCount = 0;
                other.m_capacity = 0;
            }
            return *this;
        }

        template <typename... Args>
        [[nodiscard]] T* Allocate(Args&&... args)
        {
            if (!m_freeList)
            {
                AllocateBlock();
            }

            Slot* slot = m_freeList;
            m_freeList = m_freeList->next;
            ++m_activeCount;

            T* obj = reinterpret_cast<T*>(slot->storage);
            new (obj) T(std::forward<Args>(args)...);
            return obj;
        }

        void Deallocate(T* ptr) noexcept
        {
            if (!ptr)
                return;

            ptr->~T();

            Slot* slot = reinterpret_cast<Slot*>(ptr);
            slot->next = m_freeList;
            m_freeList = slot;

            if (m_activeCount > 0)
            {
                --m_activeCount;
            }
        }

        void Reset() noexcept
        {
            m_blocks.clear();
            m_freeList = nullptr;
            m_activeCount = 0;
            m_capacity = 0;
        }

        [[nodiscard]] size_t ActiveCount() const noexcept
        {
            return m_activeCount;
        }

        [[nodiscard]] size_t Capacity() const noexcept
        {
            return m_capacity;
        }

        [[nodiscard]] bool Empty() const noexcept
        {
            return m_activeCount == 0;
        }

    private:
        void AllocateBlock()
        {
            auto block = std::make_unique<MemoryBlock>();
            for (size_t i = 0; i < BlockSize - 1; ++i)
            {
                block->slots[i].next = &block->slots[i + 1];
            }
            block->slots[BlockSize - 1].next = m_freeList;
            m_freeList = &block->slots[0];

            m_blocks.push_back(std::move(block));
            m_capacity += BlockSize;
        }

        std::vector<std::unique_ptr<MemoryBlock>> m_blocks;
        Slot* m_freeList{nullptr};
        size_t m_activeCount{0};
        size_t m_capacity{0};
    };
} // namespace EterLib::Render
