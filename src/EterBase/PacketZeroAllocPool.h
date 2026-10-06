#pragma once

#include <cstdint>
#include <array>
#include <bitset>
#include <mutex>
#include <new>
#include <utility>
#include <type_traits>
#include <cstddef>
#include "Result.h"
#include "LogModern.h"

namespace EterBase {

/**
 * @brief Thread-safe memory pool with strictly zero runtime allocation.
 * 
 * This template class reserves a fixed-size memory block at compile-time/instantiation
 * time and dispenses it. It is designed to replace `malloc` or `new` for highly frequent 
 * packet operations in TCP flows.
 * 
 * @tparam T The type of the objects to pool.
 * @tparam Capacity The maximum number of objects the pool can hold.
 */
template <typename T, std::size_t Capacity>
class PacketZeroAllocPool {
public:
    /**
     * @brief Constructs a new zero-allocation pool, initializing the free list.
     */
    PacketZeroAllocPool() noexcept {
        m_freeSlots.set(); // All slots are initially free (bit = 1)
    }

    /**
     * @brief Destroys the pool. Assumes all acquired objects have been properly released.
     */
    ~PacketZeroAllocPool() noexcept {
        // We do not auto-destroy active objects here. The caller is responsible 
        // for releasing all acquired elements to prevent resource leaks (e.g. file handles, sockets)
        // inside the type T.
    }

    // Disable copy and move
    PacketZeroAllocPool(const PacketZeroAllocPool&) = delete;
    PacketZeroAllocPool& operator=(const PacketZeroAllocPool&) = delete;
    PacketZeroAllocPool(PacketZeroAllocPool&&) = delete;
    PacketZeroAllocPool& operator=(PacketZeroAllocPool&&) = delete;

    /**
     * @brief Acquires a block of memory from the pool and constructs an object in-place.
     * 
     * @tparam Args The types of arguments to forward to T's constructor.
     * @param args Arguments to forward to the constructor of T.
     * @return PacketResult<T*> A pointer to the newly constructed object on success, or a PacketError if the pool is full.
     */
    template <typename... Args>
    [[nodiscard]] PacketResult<T*> Acquire(Args&&... args) {
        std::scoped_lock lock(m_mutex);

        // Find the first available (free) slot.
        // bitset::_Find_first() or similar is not standard cross-platform, so we iterate or use __builtin_ffs
        // Here we just loop or check for any free bit.
        
        if (m_freeSlots.none()) {
            ModernLogger::Error("PacketZeroAllocPool is out of capacity (Max: {})", Capacity);
            return MakeError(PacketError::BufferUnderflow); // Or another appropriate error
        }
        
        // Naive find first free slot
        std::size_t index = Capacity;
        for (std::size_t i = 0; i < Capacity; ++i) {
            if (m_freeSlots.test(i)) {
                index = i;
                break;
            }
        }

        if (index == Capacity) {
             return MakeError(PacketError::BufferUnderflow);
        }

        m_freeSlots.reset(index);
        
        void* ptr = &m_storage[index];

        try {
            return new (ptr) T(std::forward<Args>(args)...);
        } catch (...) {
            m_freeSlots.set(index);
            ModernLogger::Error("Exception thrown during placement new in PacketZeroAllocPool");
            throw; // Re-throw to allow higher layers to handle constructor exceptions
        }
    }

    /**
     * @brief Releases an object back into the pool.
     * 
     * The destructor of the object is explicitly called, and the memory slot is marked as free.
     * 
     * @param ptr A pointer to the object to release.
     * @return PacketResult<void> Success, or an error if the pointer is invalid or not from this pool.
     */
    [[nodiscard]] PacketResult<void> Release(T* ptr) noexcept {
        if (!ptr) {
            return MakeError(PacketError::MalformedPayload);
        }

        auto raw_ptr = reinterpret_cast<std::byte*>(ptr);
        auto base_ptr = reinterpret_cast<std::byte*>(m_storage.data());

        // Check if the pointer belongs to this pool's memory block
        if (raw_ptr < base_ptr || raw_ptr >= base_ptr + (Capacity * sizeof(StorageElement))) {
            ModernLogger::Error("Attempted to release a pointer outside the pool's storage range.");
            return MakeError(PacketError::InvalidHeader); // Or another appropriate error for 'Invalid Pointer'
        }

        std::size_t offset = raw_ptr - base_ptr;
        
        // Ensure the pointer is aligned to our element boundaries
        if (offset % sizeof(StorageElement) != 0) {
            ModernLogger::Error("Attempted to release an unaligned pointer.");
            return MakeError(PacketError::MalformedPayload);
        }

        std::size_t index = offset / sizeof(StorageElement);

        std::scoped_lock lock(m_mutex);

        if (m_freeSlots.test(index)) {
             ModernLogger::Error("Attempted to release an already free pool slot.");
             return MakeError(PacketError::MalformedPayload); // Double free
        }

        // Call destructor
        ptr->~T();

        // Mark as free
        m_freeSlots.set(index);

        return {};
    }

    /**
     * @brief Gets the number of currently available (free) slots in the pool.
     * 
     * @return std::size_t The number of free slots.
     */
    [[nodiscard]] std::size_t AvailableCount() const noexcept {
        std::scoped_lock lock(m_mutex);
        return m_freeSlots.count();
    }

private:
    // Using aligned_storage_t equivalent to ensure proper alignment and raw byte storage
    // C++23 standard way to define raw aligned memory for type T
    struct alignas(T) StorageElement {
        std::byte data[sizeof(T)];
    };

    std::array<StorageElement, Capacity> m_storage;
    std::bitset<Capacity> m_freeSlots;
    mutable std::mutex m_mutex;
};

} // namespace EterBase
