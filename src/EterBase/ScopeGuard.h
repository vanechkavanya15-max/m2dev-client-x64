#ifndef ETERBASE_SCOPEGUARD_H
#define ETERBASE_SCOPEGUARD_H

#include <concepts>
#include <utility>
#include <type_traits>

/**
 * @file ScopeGuard.h
 * @brief Implementation of the ScopeGuard (RAII) idiom for automatic resource management.
 */

/**
 * @class ScopeGuard
 * @brief A utility class that executes a provided callable upon destruction.
 * 
 * This class implements the RAII idiom to ensure that resources are released
 * or state is restored when the object goes out of scope, even in the presence
 * of exceptions.
 * 
 * @tparam Callable Type of the callable to be executed. Must satisfy std::invocable.
 */
template <std::invocable Callable>
class ScopeGuard
{
public:
    /**
     * @brief Constructs a ScopeGuard with the given callable.
     * 
     * @param callback The callable object to execute on destruction.
     */
    constexpr explicit ScopeGuard(Callable callback)
        : callback_(std::move(callback)), active_(true)
    {
    }

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    ScopeGuard(const ScopeGuard&) = delete;

    /**
     * @brief Deleted copy assignment operator to prevent copying.
     */
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    /**
     * @brief Move constructor.
     * 
     * Transfers ownership of the callable from the source ScopeGuard.
     * 
     * @param other The source ScopeGuard to move from.
     */
    constexpr ScopeGuard(ScopeGuard&& other) noexcept(std::is_nothrow_move_constructible_v<Callable>)
        : callback_(std::move(other.callback_)), active_(other.active_)
    {
        other.dismiss();
    }

    /**
     * @brief Deleted move assignment operator.
     */
    ScopeGuard& operator=(ScopeGuard&&) = delete;

    /**
     * @brief Destructor that executes the callable if the guard is still active.
     */
    ~ScopeGuard()
    {
        if (active_)
        {
            callback_();
        }
    }

    /**
     * @brief Dismisses the ScopeGuard so that the callable will not be executed.
     */
    constexpr void dismiss() noexcept
    {
        active_ = false;
    }

private:
    Callable callback_;
    bool active_;
};

/**
 * @brief Helper function to create a ScopeGuard, leveraging template argument deduction.
 * 
 * @tparam Callable Type of the callable.
 * @param callback The callable object to execute on destruction.
 * @return ScopeGuard<std::decay_t<Callable>> The created ScopeGuard.
 */
template <typename Callable>
constexpr auto MakeScopeGuard(Callable&& callback)
{
    return ScopeGuard<std::decay_t<Callable>>(std::forward<Callable>(callback));
}

#endif // ETERBASE_SCOPEGUARD_H
