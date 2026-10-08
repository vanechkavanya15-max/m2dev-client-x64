#pragma once

#include <cstdint>
#include <string_view>
#include <format>

namespace Client::Core {

/**
 * @brief Enumy bledow domenowych operacji gildii.
 */
enum class GuildError : uint8_t {
    None = 0,
    NotAuthorized,
    GuildFull,
    InsufficientMoney,
    MemberNotFound
};

[[nodiscard]] inline std::string_view to_string(GuildError error) noexcept {
    switch (error) {
        case GuildError::None: return "GuildError::None";
        case GuildError::NotAuthorized: return "GuildError::NotAuthorized - Not authorized to perform this action";
        case GuildError::GuildFull: return "GuildError::GuildFull - The guild has reached its maximum member capacity";
        case GuildError::InsufficientMoney: return "GuildError::InsufficientMoney - Not enough money for this action";
        case GuildError::MemberNotFound: return "GuildError::MemberNotFound - The specified member was not found";
        default: return "GuildError::Unknown";
    }
}

} // namespace Client::Core

namespace Core {
    using GuildError = ::Client::Core::GuildError;
}

template <>
struct std::formatter<Client::Core::GuildError> : std::formatter<std::string_view> {
    auto format(Client::Core::GuildError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Core::to_string(err), ctx);
    }
};
