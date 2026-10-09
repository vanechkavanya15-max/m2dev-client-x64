#pragma once

#include <string_view>
#include <format>
#include <iostream>
#include <cstdlib>
#include <source_location>
#include <expected>
#include <type_traits>
#include <stdexcept>
#include <utility>

//
// Core Assert and Error Handling Guardrails for M2 Engine
// Designed for C++20/C++23 environments, CI/CD, and Headless support
//

namespace Client::Core {

    namespace detail {

        inline void ReportAssertionFailure(std::string_view expr,
            std::string_view msg,
            const std::source_location& loc = std::source_location::current()) noexcept {
            
            try {
                auto output = std::format("[ASSERT FAULT] {}:{} w funkcji '{}'\n"
                    "  Wyrazenie: {}\n"
                    "  Wiadomosc: {}\n",
                    loc.file_name(), loc.line(), loc.function_name(),
                    expr, msg);
                std::cerr << output;
            }
            catch (...) {
                // Fallback, if format throws
                std::cerr << "[ASSERT FAULT] Fatalny blad asercji: " << expr 
                          << " " << msg << '\n';
            }
        }

        inline void AbortExecution() noexcept(false) {
#ifdef M2_DISABLE_ASSERT_ABORT
            // In unit testing environments we might not want to abort immediately
            // but just print the log and throw
            throw std::runtime_error("Assertion failed, execution aborted.");
#else
            std::abort();
#endif
        }

    } // namespace detail

} // namespace Client::Core

// Makra dla asercji
#define M2_ASSERT(expr) \
    do { \
        if (!(expr)) [[unlikely]] { \
            ::Client::Core::detail::ReportAssertionFailure(#expr, "Brak komunikatu"); \
            ::Client::Core::detail::AbortExecution(); \
        } \
    } while (false)

#define M2_ASSERT_MSG(expr, msg) \
    do { \
        if (!(expr)) [[unlikely]] { \
            ::Client::Core::detail::ReportAssertionFailure(#expr, msg); \
            ::Client::Core::detail::AbortExecution(); \
        } \
    } while (false)

#define M2_UNREACHABLE() \
    do { \
        ::Client::Core::detail::ReportAssertionFailure("UNREACHABLE CODE", "Osiagnieto niedozwolona sciezke wykonania"); \
        ::Client::Core::detail::AbortExecution(); \
        std::unreachable(); \
    } while (false)

// Asercja na std::expected/Result<T, E> (wymaga naglowka <expected>)
#define M2_ASSERT_EXPECTED(result) \
    do { \
        if (!(result).has_value()) [[unlikely]] { \
            ::Client::Core::detail::ReportAssertionFailure(#result, "Wartosc std::expected zawiera blad"); \
            ::Client::Core::detail::AbortExecution(); \
        } \
    } while (false)
