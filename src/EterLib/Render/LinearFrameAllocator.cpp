#include "LinearFrameAllocator.h"

namespace EterLib::Render
{
    LinearFrameAllocator::LinearFrameAllocator(size_t capacity)
        : m_capacity(capacity > 0 ? capacity : DEFAULT_CAPACITY)
        , m_offset(0)
    {
        m_buffer = std::make_unique<std::byte[]>(m_capacity);
    }

    LinearFrameAllocator::~LinearFrameAllocator() = default;

    LinearFrameAllocator::LinearFrameAllocator(LinearFrameAllocator&& other) noexcept
        : m_buffer(std::move(other.m_buffer))
        , m_capacity(other.m_capacity)
        , m_offset(other.m_offset)
    {
        other.m_capacity = 0;
        other.m_offset = 0;
    }

    LinearFrameAllocator& LinearFrameAllocator::operator=(LinearFrameAllocator&& other) noexcept
    {
        if (this != &other)
        {
            m_buffer = std::move(other.m_buffer);
            m_capacity = other.m_capacity;
            m_offset = other.m_offset;

            other.m_capacity = 0;
            other.m_offset = 0;
        }
        return *this;
    }

    void* LinearFrameAllocator::Allocate(size_t size, size_t alignment) noexcept
    {
        if (size == 0 || !m_buffer)
        {
            return nullptr;
        }

        void* ptr = m_buffer.get() + m_offset;
        size_t space = m_capacity - m_offset;

        // Use std::align to check if we can fit the object with the required alignment
        void* alignedPtr = std::align(alignment, size, ptr, space);

        if (!alignedPtr)
        {
            // Out of bounds, not enough space
            return nullptr;
        }

        // Calculate the exact memory used including any alignment padding
        // ptr was advanced by std::align, space was decreased by the padding
        // However, a simpler way is: ptr now points to the aligned memory.
        // The difference between alignedPtr and the start of buffer is the new offset
        size_t newOffset = static_cast<std::byte*>(alignedPtr) - m_buffer.get() + size;
        
        m_offset = newOffset;

        return alignedPtr;
    }

    void LinearFrameAllocator::Reset() noexcept
    {
        m_offset = 0;
    }

    size_t LinearFrameAllocator::GetAllocatedBytes() const noexcept
    {
        return m_offset;
    }

    size_t LinearFrameAllocator::GetCapacity() const noexcept
    {
        return m_capacity;
    }

} // namespace EterLib::Render

