#pragma once

#include <expected>
#include <functional>
#include <string>
#include <string_view>
#include <exception>

namespace Client::Bridge {

/// @brief Klasa chroniaca aplikacje przed wyjatkami (np. z Pythona).
/// Lapie std::exception i cokolwiek innego, zeby chronic C++ przed crashem,
/// i rzutuje wyniki na std::expected dla C++23.
class PyErrorBoundary {
public:
    enum class ErrorType {
        None,
        StdException,
        UnknownException
    };

    struct ErrorInfo {
        ErrorType type = ErrorType::None;
        std::string message;
    };

    /// @brief Wykonuje akcje, rzutujac bledy w razie awarii.
    template <typename Ret = void, typename Func>
    [[nodiscard]] static std::expected<Ret, ErrorInfo> Execute(Func&& func) noexcept {
        try {
            if constexpr (std::is_same_v<Ret, void>) {
                std::forward<Func>(func)();
                return {};
            } else {
                return std::forward<Func>(func)();
            }
        } catch (const std::exception& ex) {
            return std::unexpected(ErrorInfo{ErrorType::StdException, ex.what()});
        } catch (...) {
            return std::unexpected(ErrorInfo{ErrorType::UnknownException, "Unknown exception caught"});
        }
    }
};

} // namespace Client::Bridge
