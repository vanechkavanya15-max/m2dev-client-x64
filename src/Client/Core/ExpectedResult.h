#pragma once

#include <expected>
#include <type_traits>
#include <utility>

namespace Client::Core {

    /**
     * Wzorzec ExpectedResult implementujacy mechanizm Result<T, E> oparty na std::expected (C++23)
     * bez rzucania wyjatkow (noexcept). Pozwala na monadyczne lancuchowanie bledow.
     */
    template <typename T, typename E>
    using ExpectedResult = std::expected<T, E>;

    /**
     * Zwraca wartosc pomyslna w postaci expected
     */
    template <typename T, typename E>
    constexpr auto MakeExpected(T&& value) noexcept {
        return std::expected<std::decay_t<T>, E>(std::forward<T>(value));
    }

    /**
     * Zwraca blad
     */
    template <typename E>
    constexpr auto MakeUnexpected(E&& error) noexcept {
        return std::unexpected<std::decay_t<E>>(std::forward<E>(error));
    }

} // namespace Client::Core
