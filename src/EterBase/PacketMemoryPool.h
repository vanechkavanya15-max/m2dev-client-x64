#pragma once

#include <cstdint>
#include <vector>
#include <mutex>
#include <memory>
#include <stdexcept>
#include <utility>
#include <span>
#include <string_view>
#include <new>

/**
 * @brief A thread-safe memory pool for pre-allocated packet objects.
 * 
 * This class provides a pre-allocated memory pool to prevent heap fragmentation 
 * during frequent packet creation and destruction in network flows.
 * 
 * @tparam T The type of the object to be pooled.
 */
template <typename T>
class PacketMemoryPool {
public:
    /**
     * @brief Constructs the memory pool and pre-allocates a specified number of objects.
     * 
     * @param initial_capacity The number of objects to pre-allocate. Default is 1024.
     * @param expansion_size The number of objects to allocate when the pool runs out. Default is 1024.
     */
    explicit PacketMemoryPool(std::size_t initial_capacity = 1024, std::size_t expansion_size = 1024)
        : expansion_step(expansion_size) {
        if (initial_capacity > 0) {
            expand_pool(initial_capacity);
        }
    }

    /**
     * @brief Destroys the memory pool, freeing all allocated memory blocks.
     */
    ~PacketMemoryPool() {
        std::scoped_lock lock(mutex);
        for (void* block : blocks) {
            ::operator delete(block, std::align_val_t{alignof(T)});
        }
    }

    // Disable copy and move semantics to ensure pool integrity.
    PacketMemoryPool(const PacketMemoryPool&) = delete;
    PacketMemoryPool& operator=(const PacketMemoryPool&) = delete;
    PacketMemoryPool(PacketMemoryPool&&) = delete;
    PacketMemoryPool& operator=(PacketMemoryPool&&) = delete;

    /**
     * @brief Acquires an object from the memory pool.
     * 
     * If the pool is empty, it dynamically expands by allocating a new block.
     * The acquired object is constructed in-place using the provided arguments.
     * 
     * @tparam Args The types of the arguments passed to the constructor of T.
     * @param args The arguments passed to the constructor of T.
     * @return T* Pointer to the initialized object.
     * @throws std::bad_alloc if memory allocation for expansion fails.
     * @throws std::exception Any exception thrown by T's constructor.
     */
    template <typename... Args>
    T* acquire(Args&&... args) {
        void* raw_memory = nullptr;

        {
            std::scoped_lock lock(mutex);
            if (free_list.empty()) {
                expand_pool(expansion_step);
            }

            raw_memory = free_list.back();
            free_list.pop_back();
        }

        try {
            return new (raw_memory) T(std::forward<Args>(args)...);
        } catch (...) {
            // If constructor throws, return the raw memory back to the pool
            std::scoped_lock lock(mutex);
            free_list.push_back(raw_memory);
            throw;
        }
    }

    /**
     * @brief Releases an object back to the memory pool.
     * 
     * The destructor of the object is explicitly called, and its memory space 
     * is returned to the pool for future reuse.
     * 
     * @param instance Pointer to the object to be released. If nullptr, does nothing.
     */
    void release(T* instance) noexcept {
        if (!instance) {
            return;
        }

        // Call the destructor explicitly
        instance->~T();

        std::scoped_lock lock(mutex);
        free_list.push_back(instance);
    }

    /**
     * @brief Returns the number of currently available objects in the pool.
     * 
     * @return std::size_t The number of available objects.
     */
    std::size_t available_count() const noexcept {
        std::scoped_lock lock(mutex);
        return free_list.size();
    }

    /**
     * @brief Clears the free list and deallocates all memory blocks.
     * 
     * Note: This method does not call destructors for objects currently in use.
     * It is the caller's responsibility to ensure no acquired objects are being used
     * before calling this method.
     */
    void purge() noexcept {
        std::scoped_lock lock(mutex);
        free_list.clear();
        for (void* block : blocks) {
            ::operator delete(block, std::align_val_t{alignof(T)});
        }
        blocks.clear();
    }

private:
    /**
     * @brief Expands the memory pool by allocating a new contiguous block of memory.
     * 
     * @param capacity The number of objects to allocate space for in the new block.
     * @throws std::bad_alloc if memory allocation fails.
     */
    void expand_pool(std::size_t capacity) {
        if (capacity == 0) {
            return;
        }

        void* new_block = ::operator new(capacity * sizeof(T), std::align_val_t{alignof(T)});
        if (!new_block) {
            throw std::bad_alloc();
        }

        blocks.push_back(new_block);

        auto* current = static_cast<uint8_t*>(new_block);
        free_list.reserve(free_list.size() + capacity);
        for (std::size_t i = 0; i < capacity; ++i) {
            free_list.push_back(current + i * sizeof(T));
        }
    }

    std::vector<void*> blocks;
    std::vector<void*> free_list;
    std::size_t expansion_step;
    mutable std::mutex mutex;
};
