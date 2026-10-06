#pragma once

#include <cstdint>
#include <random>
#include <stdexcept>
#include <concepts>

/**
 * @brief A modern C++20 64-bit random number generator using std::mt19937_64.
 * 
 * This class provides uniform distributions for both integral and floating-point
 * types. It strictly uses standard types and avoids legacy Win32 types and 
 * Hungarian notation.
 */
class RandomGen64 {
public:
    /**
     * @brief Constructs the generator and seeds it with std::random_device.
     */
    RandomGen64() {
        std::random_device rd;
        engine.seed(rd());
    }

    /**
     * @brief Constructs the generator with a specific seed.
     * 
     * @param seed The seed value to initialize the random engine.
     */
    explicit RandomGen64(uint64_t seed) {
        engine.seed(seed);
    }

    /**
     * @brief Generates a uniformly distributed integer in the range [min, max].
     * 
     * @tparam T An integral type.
     * @param min The minimum value (inclusive).
     * @param max The maximum value (inclusive).
     * @return A random integer of type T.
     * @throws std::invalid_argument if min > max.
     */
    template <std::integral T>
    T GenerateRange(T min, T max) {
        if (min > max) {
            throw std::invalid_argument("min cannot be greater than max");
        }
        std::uniform_int_distribution<T> dist(min, max);
        return dist(engine);
    }

    /**
     * @brief Generates a uniformly distributed floating-point number in the range [min, max).
     * 
     * @tparam T A floating-point type.
     * @param min The minimum value (inclusive).
     * @param max The maximum value (exclusive).
     * @return A random floating-point number of type T.
     * @throws std::invalid_argument if min >= max.
     */
    template <std::floating_point T>
    T GenerateRange(T min, T max) {
        if (min >= max) {
            throw std::invalid_argument("min must be less than max for floating point ranges");
        }
        std::uniform_real_distribution<T> dist(min, max);
        return dist(engine);
    }

private:
    std::mt19937_64 engine;
};
