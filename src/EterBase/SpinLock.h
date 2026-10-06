#pragma once

#include <atomic>

/**
 * @brief A lightweight SpinLock implementation using std::atomic_flag.
 * 
 * This class provides a simple and efficient mechanism to protect 
 * shared resources. It uses a Test-and-Test-and-Set (TTAS) strategy 
 * for improved performance on multi-core systems by reducing cache 
 * coherence traffic.
 */
class SpinLock
{
public:
    /**
     * @brief Acquires the lock, spinning until it becomes available.
     * 
     * Uses std::memory_order_acquire for the synchronization, and
     * relaxed memory order for the spin loop to avoid cache line bouncing.
     */
    void lock() noexcept
    {
        for (;;)
        {
            if (!flag.test_and_set(std::memory_order_acquire))
            {
                break;
            }
            
            while (flag.test(std::memory_order_relaxed))
            {
                // Spin until the flag might be cleared.
            }
        }
    }

    /**
     * @brief Releases the lock.
     * 
     * Clears the atomic flag using std::memory_order_release to ensure
     * that all memory operations before this call are visible to other threads.
     */
    void unlock() noexcept
    {
        flag.clear(std::memory_order_release);
    }

    /**
     * @brief Attempts to acquire the lock without blocking.
     * 
     * @return true if the lock was successfully acquired, false otherwise.
     */
    bool try_lock() noexcept
    {
        return !flag.test_and_set(std::memory_order_acquire);
    }

private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};
