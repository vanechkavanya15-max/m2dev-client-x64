#pragma once

#include <cstdint>
#include <string_view>
#include <format>
#include <expected>

namespace UserInterface::Exchange {

enum class ExchangeError : uint8_t {
    None = 0,
    InvalidPosition = 1
};

[[nodiscard]] inline std::string_view to_string(ExchangeError error) noexcept {
    switch (error) {
        case ExchangeError::None: return "ExchangeError::None";
        case ExchangeError::InvalidPosition: return "ExchangeError::InvalidPosition";
        default: return "ExchangeError::Unknown";
    }
}

} // namespace UserInterface::Exchange

template <>
struct std::formatter<UserInterface::Exchange::ExchangeError> : std::formatter<std::string_view> {
    auto format(UserInterface::Exchange::ExchangeError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(UserInterface::Exchange::to_string(err), ctx);
    }
};
