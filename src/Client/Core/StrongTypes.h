#pragma once

#include <cstdint>
#include <utility>
#include <compare>
#include <format>
#include <functional>

namespace Client::Core {

template <typename Tag, typename Underlying, auto DefaultVal>
class StrongType {
public:
    using UnderlyingType = Underlying;

    constexpr StrongType() noexcept : value_{DefaultVal} {}
    constexpr explicit StrongType(Underlying value) noexcept : value_{std::move(value)} {}

    template <typename Other>
        requires (requires(const Other& o) { { o.value() } -> std::convertible_to<Underlying>; } && !std::same_as<Other, StrongType>)
    constexpr explicit StrongType(const Other& other) noexcept : value_(static_cast<Underlying>(other.value())) {}
    
    constexpr Underlying get() const noexcept { return value_; }
    constexpr Underlying value() const noexcept { return value_; }
    constexpr Underlying Value() const noexcept { return value_; }
    constexpr Underlying Get() const noexcept { return value_; }
    
    constexpr auto operator<=>(const StrongType&) const = default;
    constexpr bool operator==(const StrongType&) const = default;

    constexpr bool operator==(Underlying val) const noexcept { return value_ == val; }
    constexpr auto operator<=>(Underlying val) const noexcept { return value_ <=> val; }

    template <typename Other>
        requires (requires(const Other& o) { { o.value() } -> std::equality_comparable_with<Underlying>; } && !std::same_as<Other, StrongType>)
    constexpr bool operator==(const Other& other) const noexcept { return value_ == other.value(); }

    [[nodiscard]] constexpr explicit operator Underlying() const noexcept { return value_; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return value_ != DefaultVal; }

    constexpr StrongType& operator=(Underlying val) noexcept {
        value_ = val;
        return *this;
    }
    
private:
    Underlying value_;
};

struct EntityVidTag {};
using EntityVid = StrongType<EntityVidTag, uint32_t, 0>;
using EntityId = EntityVid;
using ActorVID = EntityVid;

struct RaceVnumTag {};
using RaceVnum = StrongType<RaceVnumTag, uint32_t, 0>;

struct ItemVnumTag {};
using ItemVnum = StrongType<ItemVnumTag, uint32_t, 0>;

struct SkillIndexTag {};
using SkillIndex = StrongType<SkillIndexTag, uint32_t, 0>;
using SkillId = SkillIndex;

struct GuildIdTag {};
using GuildId = StrongType<GuildIdTag, uint32_t, 0>;

struct SlotIndexTag {};
using SlotIndex = StrongType<SlotIndexTag, uint16_t, 0xFFFF>;
using ItemSlot = SlotIndex;

struct Money64Tag {};
using Money64 = StrongType<Money64Tag, int64_t, 0>;
using Gold = Money64;

struct MapCoords {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    
    constexpr auto operator<=>(const MapCoords&) const = default;
    
    constexpr MapCoords operator+(const MapCoords& other) const noexcept { return {x + other.x, y + other.y, z + other.z}; }
    constexpr MapCoords operator-(const MapCoords& other) const noexcept { return {x - other.x, y - other.y, z - other.z}; }
    constexpr MapCoords operator*(float scalar) const noexcept { return {x * scalar, y * scalar, z * scalar}; }
    constexpr MapCoords operator/(float scalar) const noexcept { return {x / scalar, y / scalar, z / scalar}; }
    
    constexpr MapCoords& operator+=(const MapCoords& other) noexcept { x += other.x; y += other.y; z += other.z; return *this; }
    constexpr MapCoords& operator-=(const MapCoords& other) noexcept { x -= other.x; y -= other.y; z -= other.z; return *this; }
    constexpr MapCoords& operator*=(float scalar) noexcept { x *= scalar; y *= scalar; z *= scalar; return *this; }
    constexpr MapCoords& operator/=(float scalar) noexcept { x /= scalar; y /= scalar; z /= scalar; return *this; }
    
    constexpr float Dot(const MapCoords& other) const noexcept { return x * other.x + y * other.y + z * other.z; }
    constexpr MapCoords Cross(const MapCoords& other) const noexcept {
        return {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        };
    }
    
    float Length() const noexcept;
    float Distance(const MapCoords& other) const noexcept;
    MapCoords Normalize() const noexcept;
};

} // namespace Client::Core

template <typename Tag, typename Underlying, auto DefaultVal>
struct std::hash<Client::Core::StrongType<Tag, Underlying, DefaultVal>> {
    std::size_t operator()(const Client::Core::StrongType<Tag, Underlying, DefaultVal>& obj) const noexcept {
        return std::hash<Underlying>{}(obj.get());
    }
};

template <>
struct std::hash<Client::Core::MapCoords> {
    std::size_t operator()(const Client::Core::MapCoords& obj) const noexcept;
};

template <typename Tag, typename Underlying, auto DefaultVal, typename CharT>
struct std::formatter<Client::Core::StrongType<Tag, Underlying, DefaultVal>, CharT> {
    std::formatter<Underlying, CharT> underlying_formatter;

    constexpr auto parse(auto& ctx) {
        return underlying_formatter.parse(ctx);
    }

    auto format(const Client::Core::StrongType<Tag, Underlying, DefaultVal>& obj, auto& ctx) const {
        return underlying_formatter.format(obj.get(), ctx);
    }
};

template <typename CharT>
struct std::formatter<Client::Core::MapCoords, CharT> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') throw std::format_error("invalid format");
        return it;
    }

    auto format(const Client::Core::MapCoords& coords, auto& ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {})", coords.x, coords.y, coords.z);
    }
};
