#pragma once

#include <cstdint>
#include <optional>
#include <concepts>
#include <utility>
#include "StrongTypes.h"

namespace Client::Core {

template <typename Target, typename Source>
    requires std::integral<Target> && std::integral<Source>
[[nodiscard]] constexpr std::optional<Target> SafeCast(Source val) noexcept {
    if (std::in_range<Target>(val)) {
        return static_cast<Target>(val);
    }
    return std::nullopt;
}

[[nodiscard]] constexpr bool IsValidVnum(uint32_t vnum) noexcept {
    return vnum > 0 && vnum < 10000000;
}

[[nodiscard]] constexpr bool IsValidSlot(uint16_t slot, uint16_t maxSlots = 180) noexcept {
    return slot < maxSlots;
}

[[nodiscard]] constexpr bool IsValidVID(uint32_t vid) noexcept {
    return vid != 0;
}

[[nodiscard]] constexpr std::optional<SlotIndex> ToSlotIndex(uint16_t slot) noexcept {
    if (IsValidSlot(slot)) {
        return SlotIndex{slot};
    }
    return std::nullopt;
}

[[nodiscard]] constexpr std::optional<ItemVnum> ToItemVnum(uint32_t vnum) noexcept {
    if (IsValidVnum(vnum)) {
        return ItemVnum{vnum};
    }
    return std::nullopt;
}

[[nodiscard]] constexpr std::optional<EntityVid> ToEntityVid(uint32_t vid) noexcept {
    if (IsValidVID(vid)) {
        return EntityVid{vid};
    }
    return std::nullopt;
}

} // namespace Client::Core
